# Copyright 2020 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================
"""adam"""
import numpy as np

from mindspore.common import dtype as mstype
from mindspore.common.initializer import initializer
from mindspore.ops import operations as P
from mindspore.ops import composite as C
from mindspore.ops import functional as F
from mindspore.common.parameter import Parameter
from mindspore.common.tensor import Tensor
from mindspore._checkparam import Validator as validator
from mindspore._checkparam import Rel
from .optimizer import Optimizer

_adam_opt = C.MultitypeFuncGraph("adam_opt")
_adam_push_pull_opt = C.MultitypeFuncGraph("_adam_push_pull_opt")


@_adam_opt.register("Tensor", "Tensor", "Tensor", "Tensor", "Number", "Tensor", "Tensor", "Tensor",
                    "Tensor", "Bool", "Bool")
def _update_run_op(beta1, beta2, eps, lr, weight_decay, param, m, v, gradient, decay_flag, optim_filter):
    """
    Update parameters.

    Args:
        beta1 (Tensor): The exponential decay rate for the 1st moment estimates. Should be in range (0.0, 1.0).
        beta2 (Tensor): The exponential decay rate for the 2nd moment estimates. Should be in range (0.0, 1.0).
        eps (Tensor): Term added to the denominator to improve numerical stability. Should be greater than 0.
        lr (Tensor): Learning rate.
        weight_decay (Number): Weight decay. Should be in range [0.0, 1.0].
        param (Tensor): Parameters.
        m (Tensor): m value of parameters.
        v (Tensor): v value of parameters.
        gradient (Tensor): Gradient of parameters.
        decay_flag (bool): Applies weight decay or not.
        optim_filter (bool): Applies parameter update or not.

    Returns:
        Tensor, the new value of v after updating.
    """
    if optim_filter:
        op_mul = P.Mul()
        op_square = P.Square()
        op_sqrt = P.Sqrt()
        op_cast = P.Cast()
        op_reshape = P.Reshape()
        op_shape = P.Shape()

        param_fp32 = op_cast(param, mstype.float32)
        m_fp32 = op_cast(m, mstype.float32)
        v_fp32 = op_cast(v, mstype.float32)
        gradient_fp32 = op_cast(gradient, mstype.float32)

        next_m = op_mul(beta1, m_fp32) + op_mul(op_cast(F.tuple_to_array((1.0,)), mstype.float32)
                                                - beta1, gradient_fp32)

        next_v = op_mul(beta2, v_fp32) + op_mul(op_cast(F.tuple_to_array((1.0,)), mstype.float32)
                                                - beta2, op_square(gradient_fp32))

        update = next_m / (eps + op_sqrt(next_v))
        if decay_flag:
            update = op_mul(weight_decay, param_fp32) + update

        update_with_lr = op_mul(lr, update)
        next_param = param_fp32 - op_reshape(update_with_lr, op_shape(param_fp32))

        next_param = F.depend(next_param, F.assign(param, op_cast(next_param, F.dtype(param))))
        next_param = F.depend(next_param, F.assign(m, op_cast(next_m, F.dtype(m))))
        next_param = F.depend(next_param, F.assign(v, op_cast(next_v, F.dtype(v))))
        return next_param
    return gradient


@_adam_opt.register("Function", "Function", "Tensor", "Tensor", "Tensor", "Tensor", "Number", "Tensor", "IndexedSlices",
                    "Tensor", "Tensor", "Tensor", "Bool")
def _run_opt_with_sparse(opt, sparse_opt, beta1_power, beta2_power, beta1, beta2, eps, lr, gradient, params,
                         moment1, moment2, ps_parameter):
    """Apply sparse adam optimizer to the weight parameter when the gradient is sparse."""
    success = True
    indices = gradient.indices()
    values = gradient.values()
    if ps_parameter:
        op_shape = P.Shape()
        _ps_pull = P.Pull()
        _ps_push = P.Push("Adam", [0, 1, 2])
        shapes = (op_shape(params), op_shape(moment1), op_shape(moment2),
                  op_shape(beta1_power), op_shape(beta2_power), op_shape(lr), op_shape(beta1),
                  op_shape(beta2), op_shape(eps), op_shape(values), op_shape(indices))
        success = F.depend(success, _ps_pull(_ps_push((beta1_power, beta2_power, lr, beta1, beta2,
                                                       eps, values, indices), shapes), params))
    else:
        success = F.depend(success, sparse_opt(params, moment1, moment2, beta1_power, beta2_power, lr, beta1, beta2,
                                               eps, values, indices))
    return success


@_adam_opt.register("Function", "Function", "Tensor", "Tensor", "Tensor", "Tensor", "Number", "Tensor", "Tensor",
                    "Tensor", "Tensor", "Tensor", "Bool")
def _run_opt_with_one_number(opt, sparse_opt, beta1_power, beta2_power, beta1, beta2, eps, lr, gradient, params,
                             moment1, moment2, ps_parameter):
    """Apply adam optimizer to the weight parameter using Tensor."""
    success = True
    if ps_parameter:
        op_shape = P.Shape()
        _ps_pull = P.Pull()
        _ps_push = P.Push("Adam", [0, 1, 2])
        success = F.depend(success, _ps_pull(_ps_push((beta1_power, beta2_power, lr, beta1, beta2, eps, gradient),
                                                      (op_shape(params), op_shape(moment1), op_shape(moment2))),
                                             params))
    else:
        success = F.depend(success, opt(params, moment1, moment2, beta1_power, beta2_power, lr, beta1, beta2,
                                        eps, gradient))
    return success


@_adam_push_pull_opt.register("Function", "Function", "Tensor", "Tensor", "Tensor", "Tensor", "Tensor",
                              "Tensor", "IndexedSlices", "Tensor", "Tensor", "Tensor")
def _run_push_pull_opt_with_sparse(push, pull, beta1_power, beta2_power, beta1, beta2, eps, lr, gradient, params,
                                   moment1, moment2):
    """Apply sparse adam optimizer by push and pull to the weight parameter when the gradient is sparse."""
    success = True
    op_shape = P.Shape()
    values = gradient.values()
    indices = gradient.indices()
    shapes = (op_shape(params), op_shape(moment1), op_shape(moment2),
              op_shape(beta1_power), op_shape(beta2_power), op_shape(lr), op_shape(beta1),
              op_shape(beta2), op_shape(eps), op_shape(values), op_shape(indices))
    success = F.depend(success, pull(push((beta1_power, beta2_power, lr, beta1, beta2,
                                           eps, values, indices), shapes), params))
    return success


@_adam_push_pull_opt.register("Function", "Function", "Tensor", "Tensor", "Tensor", "Tensor", "Tensor",
                              "Tensor", "Tensor", "Tensor", "Tensor", "Tensor")
def _run_push_pull_opt_with_one_number(push, pull, beta1_power, beta2_power, beta1, beta2, eps, lr, gradient, params,
                                       moment1, moment2):
    """Apply adam optimizer by push and pull to the weight parameter using Tensor."""
    success = True
    op_shape = P.Shape()
    success = F.depend(success, pull(push((beta1_power, beta2_power, lr, beta1, beta2, eps, gradient),
                                          (op_shape(params), op_shape(moment1), op_shape(moment2))), params))
    return success


def _check_param_value(beta1, beta2, eps, prim_name):
    """Check the type of inputs."""
    validator.check_value_type("beta1", beta1, [float], prim_name)
    validator.check_value_type("beta2", beta2, [float], prim_name)
    validator.check_value_type("eps", eps, [float], prim_name)
    validator.check_number_range("beta1", beta1, 0.0, 1.0, Rel.INC_NEITHER, prim_name)
    validator.check_number_range("beta2", beta2, 0.0, 1.0, Rel.INC_NEITHER, prim_name)
    validator.check_number_range("eps", eps, 0.0, float("inf"), Rel.INC_NEITHER, prim_name)


class Adam(Optimizer):
    r"""
    Updates gradients by Adaptive Moment Estimation (Adam) algorithm.

    The Adam algorithm is proposed in `Adam: A Method for Stochastic Optimization <https://arxiv.org/abs/1412.6980>`_.

    The updating formulas are as follows,

    .. math::
        \begin{array}{ll} \\
            m = \beta_1 * m + (1 - \beta_1) * g \\
            v = \beta_2 * v + (1 - \beta_2) * g * g \\
            l = \alpha * \frac{\sqrt{1-\beta_2^t}}{1-\beta_1^t} \\
            w = w - l * \frac{m}{\sqrt{v} + \epsilon}
        \end{array}

    :math:`m` represents the 1st moment vector `moment1`, :math:`v` represents the 2nd moment vector `moment2`,
    :math:`g` represents `gradients`, :math:`l` represents scaling factor `lr`, :math:`\beta_1, \beta_2` represent
    `beta1` and `beta2`, :math:`t` represents updating step while :math:`beta_1^t` and :math:`beta_2^t` represent
    `beta1_power` and `beta2_power`, :math:`\alpha` represents `learning_rate`, :math:`w` represents `params`,
    :math:`\epsilon` represents `eps`.

    Note:
        When separating parameter groups, the weight decay in each group will be applied on the parameters if the
        weight decay is positive. When not separating parameter groups, the `weight_decay` in the API will be applied
        on the parameters without 'beta' or 'gamma' in their names if `weight_decay` is positive.

        To improve parameter groups performance, the customized order of parameters can be supported.

        The sparse strategy is applied while the SparseGatherV2 operator being used for forward network.
        The sparse feature is under continuous development. The sparse
        behavior is currently performed on the CPU.

    Args:
        params (Union[list[Parameter], list[dict]]): When the `params` is a list of `Parameter` which will be updated,
            the element in `params` should be class `Parameter`. When the `params` is a list of `dict`, the "params",
            "lr", "weight_decay" and "order_params" are the keys can be parsed.

            - params: Required. The value should be a list of `Parameter`.

            - lr: Optional. If "lr" in the keys, the value of corresponding learning rate will be used.
              If not, the `learning_rate` in the API will be used.

            - weight_decay: Optional. If "weight_decay" in the keys, the value of corresponding weight decay
              will be used. If not, the `weight_decay` in the API will be used.

            - order_params: Optional. If "order_params" in the keys, the value should be the order of parameters and
              the order will be followed in optimizer. There are no other keys in the `dict` and the parameters which
              in the value of 'order_params' should be in one of group parameters.

        learning_rate (Union[float, Tensor, Iterable, LearningRateSchedule]): A value or graph for the learning rate.
            When the learning_rate is a Iterable or a Tensor with dimension of 1, use dynamic learning rate, then
            the i-th step will take the i-th value as the learning rate. When the learning_rate is LearningRateSchedule,
            use dynamic learning rate, the i-th learning rate will be calculated during the process of training
            according to the formula of LearningRateSchedule. When the learning_rate is a float or a Tensor with
            dimension of 0, use fixed learning rate. Other cases are not supported. The float learning rate should be
            equal to or greater than 0. If the type of `learning_rate` is int, it will be converted to float.
            Default: 1e-3.
        beta1 (float): The exponential decay rate for the 1st moment estimates. Should be in range (0.0, 1.0). Default:
                       0.9.
        beta2 (float): The exponential decay rate for the 2nd moment estimates. Should be in range (0.0, 1.0). Default:
                       0.999.
        eps (float): Term added to the denominator to improve numerical stability. Should be greater than 0. Default:
                     1e-8.
        use_locking (bool): Whether to enable a lock to protect updating variable tensors.
            If True, updating of the var, m, and v tensors will be protected by a lock.
            If False, the result is unpredictable. Default: False.
        use_nesterov (bool): Whether to use Nesterov Accelerated Gradient (NAG) algorithm to update the gradients.
            If True, updates the gradients using NAG.
            If False, updates the gradients without using NAG. Default: False.
        weight_decay (float): Weight decay (L2 penalty). It should be in range [0.0, 1.0]. Default: 0.0.
        loss_scale (float): A floating point value for the loss scale. Should be not less than 1.0. Default: 1.0.

    Inputs:
        - **gradients** (tuple[Tensor]) - The gradients of `params`, the shape is the same as `params`.

    Outputs:
        Tensor[bool], the value is True.

    Examples:
        >>> net = Net()
        >>> #1) All parameters use the same learning rate and weight decay
        >>> optim = nn.Adam(params=net.trainable_params())
        >>>
        >>> #2) Use parameter groups and set different values
        >>> conv_params = list(filter(lambda x: 'conv' in x.name, net.trainable_params()))
        >>> no_conv_params = list(filter(lambda x: 'conv' not in x.name, net.trainable_params()))
        >>> group_params = [{'params': conv_params, 'weight_decay': 0.01},
        >>>                 {'params': no_conv_params, 'lr': 0.01},
        >>>                 {'order_params': net.trainable_params()}]
        >>> optm = nn.Adam(group_params, learning_rate=0.1, weight_decay=0.0)
        >>> # The conv_params's parameters will use default learning rate of 0.1 and weight decay of 0.01.
        >>> # The no_conv_params's parameters will use learning rate of 0.01 and defaule weight decay of 0.0.
        >>> # The final parameters order in which the optimizer will be followed is the value of 'order_params'.
        >>>
        >>> loss = nn.SoftmaxCrossEntropyWithLogits()
        >>> model = Model(net, loss_fn=loss, optimizer=optim)
    """

    def __init__(self, params, learning_rate=1e-3, beta1=0.9, beta2=0.999, eps=1e-8, use_locking=False,
                 use_nesterov=False, weight_decay=0.0, loss_scale=1.0):
        super(Adam, self).__init__(learning_rate, params, weight_decay, loss_scale)
        _check_param_value(beta1, beta2, eps, self.cls_name)
        validator.check_value_type("use_locking", use_locking, [bool], self.cls_name)
        validator.check_value_type("use_nesterov", use_nesterov, [bool], self.cls_name)

        self.beta1 = Tensor(beta1, mstype.float32)
        self.beta2 = Tensor(beta2, mstype.float32)
        self.beta1_power = Parameter(initializer(1, [1], mstype.float32), name="beta1_power")
        self.beta2_power = Parameter(initializer(1, [1], mstype.float32), name="beta2_power")
        self.eps = eps

        self.moment1 = self.parameters.clone(prefix="moment1", init='zeros')
        self.moment2 = self.parameters.clone(prefix="moment2", init='zeros')

        self.hyper_map = C.HyperMap()
        self.opt = P.Adam(use_locking, use_nesterov)
        self.sparse_opt = P.FusedSparseAdam(use_locking, use_nesterov)

    def construct(self, gradients):
        params = self.parameters
        moment1 = self.moment1
        moment2 = self.moment2
        gradients = self.decay_weight(gradients)
        gradients = self.scale_grad(gradients)
        lr = self.get_lr()

        beta1_power = self.beta1_power * self.beta1
        self.beta1_power = beta1_power
        beta2_power = self.beta2_power * self.beta2
        self.beta2_power = beta2_power
        if self.is_group_lr:
            success = self.map_(F.partial(_adam_opt, self.opt, self.sparse_opt, beta1_power, beta2_power,
                                          self.beta1, self.beta2, self.eps),
                                lr, gradients, params, moment1, moment2, self.ps_parameters)
        else:
            success = self.map_(F.partial(_adam_opt, self.opt, self.sparse_opt, beta1_power, beta2_power,
                                          self.beta1, self.beta2, self.eps, lr),
                                gradients, params, moment1, moment2, self.ps_parameters)
        return success


class PSAdam(Optimizer):
    '''The same usage as Adam optimizer except the parameters are set PS mode.'''
    def __init__(self, params, learning_rate=1e-3, beta1=0.9, beta2=0.999, eps=1e-8, use_locking=False,
                 use_nesterov=False, weight_decay=0.0, loss_scale=1.0):
        super(PSAdam, self).__init__(learning_rate, params, weight_decay, loss_scale)
        _check_param_value(beta1, beta2, eps, self.cls_name)
        validator.check_value_type("use_locking", use_locking, [bool], self.cls_name)
        validator.check_value_type("use_nesterov", use_nesterov, [bool], self.cls_name)

        self.beta1 = Tensor(beta1, mstype.float32)
        self.beta2 = Tensor(beta2, mstype.float32)
        self.beta1_power = Parameter(initializer(1, [1], mstype.float32), name="beta1_power")
        self.beta2_power = Parameter(initializer(1, [1], mstype.float32), name="beta2_power")
        self.eps = Tensor(eps, mstype.float32)

        self.moment1 = self.parameters.clone(prefix="moment1", init='zeros')
        self.moment2 = self.parameters.clone(prefix="moment2", init='zeros')

        self.hyper_map = C.HyperMap()
        self.push = P.Push("Adam", [0, 1, 2])
        self.push.add_prim_attr("primitive_target", "CPU")
        self.pull = P.Pull()
        self.pull.add_prim_attr("primitive_target", "CPU")

    def construct(self, gradients):
        params = self.parameters
        moment1 = self.moment1
        moment2 = self.moment2
        gradients = self.decay_weight(gradients)
        gradients = self.scale_grad(gradients)
        lr = self.get_lr()

        beta1_power = self.beta1_power * self.beta1
        self.beta1_power = beta1_power
        beta2_power = self.beta2_power * self.beta2
        self.beta2_power = beta2_power
        if self.is_group_lr:
            success = self.map_(F.partial(_adam_push_pull_opt, self.push, self.pull, beta1_power, beta2_power,
                                          self.beta1, self.beta2, self.eps),
                                lr, gradients, params, moment1, moment2)
        else:
            success = self.map_(F.partial(_adam_push_pull_opt, self.push, self.pull, beta1_power, beta2_power,
                                          self.beta1, self.beta2, self.eps, lr),
                                gradients, params, moment1, moment2)
        return success


class AdamWeightDecay(Optimizer):
    """
    Implements Adam algorithm weight decay fix.

    Note:
        When separating parameter groups, the weight decay in each group will be applied on the parameters if the
        weight decay is posigive. When not separating parameter groups, the `weight_decay` in the API will be applied
        on the parameters without 'beta' or 'gamma' in their names if `weight_decay` is positive.

        To improve parameter groups performance, the customized order of parameters can be supported.

    Args:
        params (Union[list[Parameter], list[dict]]): When the `params` is a list of `Parameter` which will be updated,
            the element in `params` should be class `Parameter`. When the `params` is a list of `dict`, the "params",
            "lr", "weight_decay" and "order_params" are the keys can be parsed.

            - params: Required. The value should be a list of `Parameter`.

            - lr: Optional. If "lr" in the keys, the value of corresponding learning rate will be used.
              If not, the `learning_rate` in the API will be used.

            - weight_decay: Optional. If "weight_decay" in the keys, the value of corresponding weight decay
              will be used. If not, the `weight_decay` in the API will be used.

            - order_params: Optional. If "order_params" in the keys, the value should be the order of parameters and
              the order will be followed in optimizer. There are no other keys in the `dict` and the parameters which
              in the value of 'order_params' should be in one of group parameters.

        learning_rate (Union[float, Tensor, Iterable, LearningRateSchedule]): A value or graph for the learning rate.
            When the learning_rate is a Iterable or a Tensor with dimension of 1, use dynamic learning rate, then
            the i-th step will take the i-th value as the learning rate. When the learning_rate is LearningRateSchedule,
            use dynamic learning rate, the i-th learning rate will be calculated during the process of training
            according to the formula of LearningRateSchedule. When the learning_rate is a float or a Tensor with
            dimension of 0, use fixed learning rate. Other cases are not supported. The float learning rate should be
            equal to or greater than 0. If the type of `learning_rate` is int, it will be converted to float.
            Default: 1e-3.
        beta1 (float): The exponential decay rate for the 1st moment estimates. Default: 0.9.
            Should be in range (0.0, 1.0).
        beta2 (float): The exponential decay rate for the 2nd moment estimates. Default: 0.999.
            Should be in range (0.0, 1.0).
        eps (float): Term added to the denominator to improve numerical stability. Default: 1e-6.
            Should be greater than 0.
        weight_decay (float): Weight decay (L2 penalty). It should be in range [0.0, 1.0]. Default: 0.0.
        decay_filter (Function): A function to determine whether to apply weight decay on parameters. Default:
                                 lambda x: 'LayerNorm' not in x.name and 'bias' not in x.name.

    Inputs:
        - **gradients** (tuple[Tensor]) - The gradients of `params`, the shape is the same as `params`.

    Outputs:
        tuple[bool], all elements are True.

    Examples:
        >>> net = Net()
        >>> #1) All parameters use the same learning rate and weight decay
        >>> optim = nn.AdamWeightDecay(params=net.trainable_params())
        >>>
        >>> #2) Use parameter groups and set different values
        >>> conv_params = list(filter(lambda x: 'conv' in x.name, net.trainable_params()))
        >>> no_conv_params = list(filter(lambda x: 'conv' not in x.name, net.trainable_params()))
        >>> group_params = [{'params': conv_params, 'weight_decay': 0.01},
        >>>                 {'params': no_conv_params, 'lr': 0.01},
        >>>                 {'order_params': net.trainable_params()}]
        >>> optim = nn.AdamWeightDecay(group_params, learning_rate=0.1, weight_decay=0.0)
        >>> # The conv_params's parameters will use default learning rate of 0.1 and weight decay of 0.01.
        >>> # The no_conv_params's parameters will use learning rate of 0.01 and default weight decay of 0.0.
        >>> # The final parameters order in which the optimizer will be followed is the value of 'order_params'.
        >>>
        >>> loss = nn.SoftmaxCrossEntropyWithLogits()
        >>> model = Model(net, loss_fn=loss, optimizer=optim)
   """
    def __init__(self, params, learning_rate=1e-3, beta1=0.9, beta2=0.999, eps=1e-6, weight_decay=0.0):
        super(AdamWeightDecay, self).__init__(learning_rate, params, weight_decay)
        _check_param_value(beta1, beta2, eps, self.cls_name)
        self.beta1 = Tensor(np.array([beta1]).astype(np.float32))
        self.beta2 = Tensor(np.array([beta2]).astype(np.float32))
        self.eps = Tensor(np.array([eps]).astype(np.float32))
        self.moments1 = self.parameters.clone(prefix="adam_m", init='zeros')
        self.moments2 = self.parameters.clone(prefix="adam_v", init='zeros')
        self.hyper_map = C.HyperMap()

    def construct(self, gradients):
        lr = self.get_lr()
        if self.is_group:
            if self.is_group_lr:
                optim_result = self.hyper_map(F.partial(_adam_opt, self.beta1, self.beta2, self.eps),
                                              lr, self.weight_decay, self.parameters, self.moments1, self.moments2,
                                              gradients, self.decay_flags, self.optim_filter)
            else:
                optim_result = self.hyper_map(F.partial(_adam_opt, self.beta1, self.beta2, self.eps, lr),
                                              self.weight_decay, self.parameters, self.moments1, self.moments2,
                                              gradients, self.decay_flags, self.optim_filter)
        else:
            optim_result = self.hyper_map(F.partial(_adam_opt, self.beta1, self.beta2, self.eps, lr, self.weight_decay),
                                          self.parameters, self.moments1, self.moments2,
                                          gradients, self.decay_flags, self.optim_filter)
        if self.use_parallel:
            optim_result = self.broadcast_params(optim_result)
        return optim_result
