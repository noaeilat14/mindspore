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
"""Exponential Distribution"""
import numpy as np
from mindspore.ops import operations as P
from mindspore.ops import composite as C
from mindspore.common import dtype as mstype
from .distribution import Distribution
from ._utils.utils import check_greater_zero, check_type, check_distribution_name
from ._utils.custom_ops import exp_generic, log_generic


class Exponential(Distribution):
    """
    Example class: Exponential Distribution.

    Args:
        rate (float, list, numpy.ndarray, Tensor, Parameter): The inverse scale.
        seed (int): The seed used in sampling. The global seed is used if it is None. Default: None.
        dtype (mindspore.dtype): The type of the event samples. Default: mstype.float32.
        name (str): The name of the distribution. Default: 'Exponential'.

    Note:
        `rate` must be strictly greater than 0.
        `dist_spec_args` is `rate`.
        `dtype` must be a float type because Exponential distributions are continuous.

    Examples:
        >>> # To initialize an Exponential distribution of the rate 0.5.
        >>> import mindspore.nn.probability.distribution as msd
        >>> e = msd.Exponential(0.5, dtype=mstype.float32)
        >>>
        >>> # The following creates two independent Exponential distributions.
        >>> e = msd.Exponential([0.5, 0.5], dtype=mstype.float32)
        >>>
        >>> # An Exponential distribution can be initilized without arguments.
        >>> # In this case, `rate` must be passed in through `args` during function calls.
        >>> e = msd.Exponential(dtype=mstype.float32)
        >>>
        >>> # To use an Exponential distribution in a network.
        >>> class net(Cell):
        >>>     def __init__(self):
        >>>         super(net, self).__init__():
        >>>         self.e1 = msd.Exponential(0.5, dtype=mstype.float32)
        >>>         self.e2 = msd.Exponential(dtype=mstype.float32)
        >>>
        >>>     # All the following calls in construct are valid.
        >>>     def construct(self, value, rate_b, rate_a):
        >>>
        >>>         # Private interfaces of probability functions corresponding to public interfaces, including
        >>>         # `prob`, `log_prob`, `cdf`, `log_cdf`, `survival_function`, and `log_survival`, are the same as follows.
        >>>         # Args:
        >>>         #     value (Tensor): the value to be evaluated.
        >>>         #     rate (Tensor): the rate of the distribution. Default: self.rate.
        >>>
        >>>         # Examples of `prob`.
        >>>         # Similar calls can be made to other probability functions
        >>>         # by replacing `prob` by the name of the function.
        >>>         ans = self.e1.prob(value)
        >>>         # Evaluate with respect to distribution b.
        >>>         ans = self.e1.prob(value, rate_b)
        >>>         # `rate` must be passed in during function calls.
        >>>         ans = self.e2.prob(value, rate_a)
        >>>
        >>>
        >>>         # Functions `mean`, `sd`, 'var', and 'entropy' have the same arguments as follows.
        >>>         # Args:
        >>>         #     rate (Tensor): the rate of the distribution. Default: self.rate.
        >>>
        >>>         # Examples of `mean`. `sd`, `var`, and `entropy` are similar.
        >>>         ans = self.e1.mean() # return 2
        >>>         ans = self.e1.mean(rate_b) # return 1 / rate_b
        >>>         # `rate` must be passed in during function calls.
        >>>         ans = self.e2.mean(rate_a)
        >>>
        >>>
        >>>         # Interfaces of `kl_loss` and `cross_entropy` are the same.
        >>>         # Args:
        >>>         #     dist (str): The name of the distribution. Only 'Exponential' is supported.
        >>>         #     rate_b (Tensor): the rate of distribution b.
        >>>         #     rate_a (Tensor): the rate of distribution a. Default: self.rate.
        >>>
        >>>         # Examples of `kl_loss`. `cross_entropy` is similar.
        >>>         ans = self.e1.kl_loss('Exponential', rate_b)
        >>>         ans = self.e1.kl_loss('Exponential', rate_b, rate_a)
        >>>         # An additional `rate` must be passed in.
        >>>         ans = self.e2.kl_loss('Exponential', rate_b, rate_a)
        >>>
        >>>
        >>>         # Examples of `sample`.
        >>>         # Args:
        >>>         #     shape (tuple): the shape of the sample. Default: ()
        >>>         #     probs1 (Tensor): the rate of the distribution. Default: self.rate.
        >>>         ans = self.e1.sample()
        >>>         ans = self.e1.sample((2,3))
        >>>         ans = self.e1.sample((2,3), rate_b)
        >>>         ans = self.e2.sample((2,3), rate_a)
    """

    def __init__(self,
                 rate=None,
                 seed=None,
                 dtype=mstype.float32,
                 name="Exponential"):
        """
        Constructor of Exponential.
        """
        param = dict(locals())
        param['param_dict'] = {'rate': rate}
        valid_dtype = mstype.float_type
        check_type(dtype, valid_dtype, type(self).__name__)
        super(Exponential, self).__init__(seed, dtype, name, param)

        self._rate = self._add_parameter(rate, 'rate')
        if self.rate is not None:
            check_greater_zero(self.rate, 'rate')

        self.minval = np.finfo(np.float).tiny

        # ops needed for the class
        self.exp = exp_generic
        self.log = log_generic
        self.squeeze = P.Squeeze(0)
        self.cast = P.Cast()
        self.const = P.ScalarToArray()
        self.dtypeop = P.DType()
        self.fill = P.Fill()
        self.less = P.Less()
        self.select = P.Select()
        self.shape = P.Shape()
        self.uniform = C.uniform

    def extend_repr(self):
        if self.is_scalar_batch:
            str_info = f'rate = {self.rate}'
        else:
            str_info = f'batch_shape = {self._broadcast_shape}'
        return str_info

    @property
    def rate(self):
        """
        Return `rate` of the distribution.
        """
        return self._rate

    def _mean(self, rate=None):
        r"""
        .. math::
            MEAN(EXP) = \frac{1.0}{\lambda}.
        """
        rate = self._check_param_type(rate)
        return 1.0 / rate

    def _mode(self, rate=None):
        r"""
        .. math::
            MODE(EXP) = 0.
        """
        rate = self._check_param_type(rate)
        return self.fill(self.dtype, self.shape(rate), 0.)

    def _sd(self, rate=None):
        r"""
        .. math::
            sd(EXP) = \frac{1.0}{\lambda}.
        """
        rate = self._check_param_type(rate)
        return 1.0 / rate

    def _entropy(self, rate=None):
        r"""
        .. math::
            H(Exp) = 1 - \log(\lambda).
        """
        rate = self._check_param_type(rate)
        return 1.0 - self.log(rate)

    def _cross_entropy(self, dist, rate_b, rate=None):
        """
        Evaluate cross entropy between Exponential distributions.

        Args:
            dist (str): The type of the distributions. Should be "Exponential" in this case.
            rate_b (Tensor): The rate of distribution b.
            rate_a (Tensor): The rate of distribution a. Default: self.rate.
        """
        check_distribution_name(dist, 'Exponential')
        return self._entropy(rate) + self._kl_loss(dist, rate_b, rate)

    def _log_prob(self, value, rate=None):
        r"""
        Log probability density function of Exponential distributions.

        Args:
            Args:
            value (Tensor): The value to be evaluated.
            rate (Tensor): The rate of the distribution. Default: self.rate.

        Note:
            `value` must be greater or equal to zero.

        .. math::
            log_pdf(x) = \log(rate) - rate * x if x >= 0 else 0
        """
        value = self._check_value(value, "value")
        value = self.cast(value, self.dtype)
        rate = self._check_param_type(rate)
        prob = self.log(rate) - rate * value
        zeros = self.fill(self.dtypeop(prob), self.shape(prob), 0.0)
        neginf = self.fill(self.dtypeop(prob), self.shape(prob), -np.inf)
        comp = self.less(value, zeros)
        return self.select(comp, neginf, prob)

    def _cdf(self, value, rate=None):
        r"""
        Cumulative distribution function (cdf) of Exponential distributions.

        Args:
            value (Tensor): The value to be evaluated.
            rate (Tensor): The rate of the distribution. Default: self.rate.

        Note:
            `value` must be greater or equal to zero.

        .. math::
            cdf(x) = 1.0 - \exp(-1 * \lambda * x) if x >= 0 else 0
        """
        value = self._check_value(value, 'value')
        value = self.cast(value, self.dtype)
        rate = self._check_param_type(rate)
        cdf = 1.0 - self.exp(-1. * rate * value)
        zeros = self.fill(self.dtypeop(cdf), self.shape(cdf), 0.0)
        comp = self.less(value, zeros)
        return self.select(comp, zeros, cdf)

    def _log_survival(self, value, rate=None):
        r"""
        Log survival_function of Exponential distributions.

        Args:
            value (Tensor): The value to be evaluated.
            rate (Tensor): The rate of the distribution. Default: self.rate.

        Note:
            `value` must be greater or equal to zero.

        .. math::
            log_survival_function(x) = -1 * \lambda * x if x >= 0 else 0
        """
        value = self._check_value(value, 'value')
        value = self.cast(value, self.dtype)
        rate = self._check_param_type(rate)
        sf = -1. * rate * value
        zeros = self.fill(self.dtypeop(sf), self.shape(sf), 0.0)
        comp = self.less(value, zeros)
        return self.select(comp, zeros, sf)

    def _kl_loss(self, dist, rate_b, rate=None):
        """
        Evaluate exp-exp kl divergence, i.e. KL(a||b).

        Args:
            dist (str): The type of the distributions. Should be "Exponential" in this case.
            rate_b (Tensor): The rate of distribution b.
            rate_a (Tensor): The rate of distribution a. Default: self.rate.
        """
        check_distribution_name(dist, 'Exponential')
        rate_b = self._check_value(rate_b, 'rate_b')
        rate_b = self.cast(rate_b, self.parameter_type)
        rate_a = self._check_param_type(rate)
        return self.log(rate_a) - self.log(rate_b) + rate_b / rate_a - 1.0

    def _sample(self, shape=(), rate=None):
        """
        Sampling.

        Args:
            shape (tuple): The shape of the sample. Default: ().
            rate (Tensor): The rate of the distribution. Default: self.rate.

        Returns:
            Tensor, shape is shape + batch_shape.
        """
        shape = self.checktuple(shape, 'shape')
        rate = self._check_param_type(rate)
        origin_shape = shape + self.shape(rate)
        if origin_shape == ():
            sample_shape = (1,)
        else:
            sample_shape = origin_shape
        minval = self.const(self.minval)
        maxval = self.const(1.0)
        sample_uniform = self.uniform(sample_shape, minval, maxval, self.seed)
        sample = self.log(sample_uniform) / rate
        value = self.cast(-sample, self.dtype)
        if origin_shape == ():
            value = self.squeeze(value)
        return value
