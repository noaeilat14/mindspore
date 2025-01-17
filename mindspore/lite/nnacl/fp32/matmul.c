/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nnacl/fp32/matmul.h"

void RowMajor2Row4Major(float *src_ptr, float *dst_ptr, int row, int col) {
  for (int r = 0; r < row; r++) {
    float *src = src_ptr + r * col;
    for (int c = 0; c < col; c++) {
      int cd8 = c / 4;
      int cm8 = c % 4;
      dst_ptr[cd8 * 4 * row + r * 4 + cm8] = src[c];
    }
  }
  return;
}

void RowMajor2Row8Major(float *src_ptr, float *dst_ptr, int row, int col) {
  for (int r = 0; r < row; r++) {
    float *src = src_ptr + r * col;
    for (int c = 0; c < col; c++) {
      int cd8 = c / 8;
      int cm8 = c % 8;
      dst_ptr[cd8 * 8 * row + r * 8 + cm8] = src[c];
    }
  }
  return;
}

void RowMajor2Row12Major(float *src_ptr, float *dst_ptr, int row, int col) {
  for (int r = 0; r < row; r++) {
    float *src = src_ptr + r * col;
    for (int c = 0; c < col; c++) {
      int cd8 = c / C12NUM;
      int cm8 = c % C12NUM;
      dst_ptr[cd8 * C12NUM * row + r * C12NUM + cm8] = src[c];
    }
  }
  return;
}

void RowMajor2Col12Major(float *src_ptr, float *dst_ptr, size_t row, size_t col) {
  size_t row_up_12 = UP_ROUND(row, C12NUM);
  size_t row12 = row / C12NUM * C12NUM;
  size_t col4 = col / C4NUM * C4NUM;
  float *src_r = src_ptr;
  float *dst_r = dst_ptr;

  size_t ri = 0;
  for (; ri < row12; ri += C12NUM) {
    size_t ci = 0;
    for (; ci < col4; ci += C4NUM) {
      float *src_c = src_r + ci;
      float *dst_c = dst_r + ci * C12NUM;

      /* 12x4 row-major to col-major */
#ifdef ENABLE_ARM64
      size_t stride = col * sizeof(float);
      asm volatile(
        "mov x10, %[src_c]\n"
        "mov x11, %[dst_c]\n"

        "ld1 {v0.4s}, [x10], %[stride]\n"
        "ld1 {v1.4s}, [x10], %[stride]\n"
        "ld1 {v2.4s}, [x10], %[stride]\n"
        "ld1 {v3.4s}, [x10], %[stride]\n"

        "ld1 {v4.4s}, [x10], %[stride]\n"
        "ld1 {v5.4s}, [x10], %[stride]\n"
        "ld1 {v6.4s}, [x10], %[stride]\n"
        "ld1 {v7.4s}, [x10], %[stride]\n"

        "zip1 v12.4s, v0.4s, v1.4s\n"
        "zip2 v13.4s, v0.4s, v1.4s\n"
        "zip1 v14.4s, v2.4s, v3.4s\n"
        "zip2 v15.4s, v2.4s, v3.4s\n"

        "ld1 {v8.4s}, [x10], %[stride]\n"
        "ld1 {v9.4s}, [x10], %[stride]\n"
        "ld1 {v10.4s}, [x10], %[stride]\n"
        "ld1 {v11.4s}, [x10], %[stride]\n"

        "zip1 v16.4s, v4.4s, v5.4s\n"
        "zip2 v17.4s, v4.4s, v5.4s\n"
        "zip1 v18.4s, v6.4s, v7.4s\n"
        "zip2 v19.4s, v6.4s, v7.4s\n"

        "trn1 v20.2d, v12.2d, v14.2d\n"
        "trn2 v23.2d, v12.2d, v14.2d\n"
        "trn1 v26.2d, v13.2d, v15.2d\n"
        "trn2 v29.2d, v13.2d, v15.2d\n"

        "trn1 v21.2d, v16.2d, v18.2d\n"
        "trn2 v24.2d, v16.2d, v18.2d\n"
        "trn1 v27.2d, v17.2d, v19.2d\n"
        "trn2 v30.2d, v17.2d, v19.2d\n"

        "zip1 v12.4s, v8.4s, v9.4s\n"
        "zip2 v13.4s, v8.4s, v9.4s\n"
        "zip1 v14.4s, v10.4s, v11.4s\n"
        "zip2 v15.4s, v10.4s, v11.4s\n"

        "trn1 v22.2d, v12.2d, v14.2d\n"
        "trn2 v25.2d, v12.2d, v14.2d\n"
        "trn1 v28.2d, v13.2d, v15.2d\n"
        "trn2 v31.2d, v13.2d, v15.2d\n"

        "st1 {v20.4s, v21.4s, v22.4s, v23.4s}, [x11], #64\n"
        "st1 {v24.4s, v25.4s, v26.4s, v27.4s}, [x11], #64\n"
        "st1 {v28.4s, v29.4s, v30.4s, v31.4s}, [x11], #64\n"

        :
        : [ dst_c ] "r"(dst_c), [ src_c ] "r"(src_c), [ stride ] "r"(stride)
        : "x10", "x11", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
          "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29",
          "v30", "v31");
#elif ENABLE_ARM32
      size_t stride = col * sizeof(float);
      asm volatile(
        "mov r10, %[src_c]\n"
        "mov r12, %[dst_c]\n"

        "vld1.32 {q0}, [r10], %[stride]\n"
        "vld1.32 {q3}, [r10], %[stride]\n"
        "vld1.32 {q10}, [r10], %[stride]\n"
        "vld1.32 {q13}, [r10], %[stride]\n"

        "vtrn.32 d0, d6\n"
        "vtrn.32 d1, d7\n"
        "vtrn.32 d20, d26\n"
        "vtrn.32 d21, d27\n"

        "vld1.32 {q1}, [r10], %[stride]\n"
        "vld1.32 {q8}, [r10], %[stride]\n"
        "vld1.32 {q11}, [r10], %[stride]\n"
        "vld1.32 {q14}, [r10], %[stride]\n"

        "vswp d1, d20\n"
        "vswp d7, d26\n"

        "vld1.32 {q2}, [r10], %[stride]\n"
        "vld1.32 {q9}, [r10], %[stride]\n"
        "vld1.32 {q12}, [r10], %[stride]\n"
        "vld1.32 {q15}, [r10], %[stride]\n"

        "vtrn.32 d2, d16\n"
        "vtrn.32 d3, d17\n"
        "vtrn.32 d22, d28\n"
        "vtrn.32 d23, d29\n"

        "vswp d3, d22\n"
        "vswp d17, d28\n"

        "vtrn.32 d4, d18\n"
        "vtrn.32 d5, d19\n"
        "vtrn.32 d24, d30\n"
        "vtrn.32 d25, d31\n"

        "vswp d5, d24\n"
        "vswp d19, d30\n"

        "vst1.32 {q0, q1}, [r12]!\n"
        "vst1.32 {q2, q3}, [r12]!\n"
        "vst1.32 {q8, q9}, [r12]!\n"
        "vst1.32 {q10, q11}, [r12]!\n"
        "vst1.32 {q12, q13}, [r12]!\n"
        "vst1.32 {q14, q15}, [r12]!\n"

        :
        : [ dst_c ] "r"(dst_c), [ src_c ] "r"(src_c), [ stride ] "r"(stride)
        : "r10", "r12", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15");
#else
      for (int tr = 0; tr < C12NUM; tr++) {
        for (int tc = 0; tc < C4NUM; tc++) {
          dst_c[tc * C12NUM + tr] = src_c[tr * col + tc];
        }
      }
#endif
    }
    for (; ci < col; ci++) {
      float *src_c = src_r + ci;
      float *dst_c = dst_r + ci * C12NUM;
      for (size_t i = 0; i < C12NUM; i++) {
        dst_c[i] = src_c[i * col];
      }
    }
    src_r += C12NUM * col;
    dst_r += C12NUM * col;
  }

  for (; ri < row; ri++) {
    for (size_t i = 0; i < col; i++) {
      dst_r[i * C12NUM] = src_r[i];
    }
    src_r += col;
    dst_r += 1;
  }

  for (; ri < row_up_12; ri++) {
    for (size_t i = 0; i < col; i++) {
      dst_r[i * C12NUM] = 0;
    }
    dst_r += 1;
  }
  return;
}

void RowMajor2Col8Major(float *src_ptr, float *dst_ptr, size_t row, size_t col) {
  size_t row8 = row / C8NUM * C8NUM;
  size_t col4 = col / C4NUM * C4NUM;
  float *src_r = src_ptr;
  float *dst_r = dst_ptr;

  size_t ri = 0;
  for (; ri < row8; ri += C8NUM) {
    size_t ci = 0;
    for (; ci < col4; ci += C4NUM) {
      float *src_c = src_r + ci;
      float *dst_c = dst_r + ci * C8NUM;

      /* 8x4 row-major to col-major */
#ifdef ENABLE_ARM64
      size_t stride = col * 4;
      asm volatile(
        "mov x10, %[src_c]\n"
        "mov x11, %[dst_c]\n"

        "ld1 {v0.4s}, [x10], %[stride]\n"
        "ld1 {v1.4s}, [x10], %[stride]\n"
        "ld1 {v2.4s}, [x10], %[stride]\n"
        "ld1 {v3.4s}, [x10], %[stride]\n"

        "zip1 v4.4s, v0.4s, v1.4s\n"
        "zip2 v5.4s, v0.4s, v1.4s\n"
        "zip1 v6.4s, v2.4s, v3.4s\n"
        "zip2 v7.4s, v2.4s, v3.4s\n"

        "ld1 {v8.4s},  [x10], %[stride]\n"
        "ld1 {v9.4s},  [x10], %[stride]\n"
        "ld1 {v10.4s}, [x10],  %[stride]\n"
        "ld1 {v11.4s}, [x10],  %[stride]\n"

        "trn1 v0.2d, v4.2d, v6.2d\n"
        "trn2 v1.2d, v4.2d, v6.2d\n"
        "trn1 v2.2d, v5.2d, v7.2d\n"
        "trn2 v3.2d, v5.2d, v7.2d\n"

        "zip1 v12.4s, v8.4s, v9.4s\n"
        "zip2 v13.4s, v8.4s, v9.4s\n"
        "zip1 v14.4s, v10.4s, v11.4s\n"
        "zip2 v15.4s, v10.4s, v11.4s\n"

        "trn1 v8.2d, v12.2d, v14.2d\n"
        "trn2 v9.2d, v12.2d, v14.2d\n"
        "trn1 v10.2d, v13.2d, v15.2d\n"
        "trn2 v11.2d, v13.2d, v15.2d\n"

        "st1 {v0.4s}, [x11],  #16\n"
        "st1 {v8.4s}, [x11],  #16\n"
        "st1 {v1.4s}, [x11],  #16\n"
        "st1 {v9.4s}, [x11],  #16\n"
        "st1 {v2.4s},  [x11],#16\n"
        "st1 {v10.4s}, [x11], #16\n"
        "st1 {v3.4s},  [x11],#16\n"
        "st1 {v11.4s}, [x11], #16\n"

        :
        : [ dst_c ] "r"(dst_c), [ src_c ] "r"(src_c), [ stride ] "r"(stride)
        : "x10", "x11", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14",
          "v15");
#else
      for (int tr = 0; tr < 8; tr++) {
        for (int tc = 0; tc < 4; tc++) {
          dst_c[tc * 8 + tr] = src_c[tr * col + tc];
        }
      }
#endif
    }
    for (; ci < col; ci++) {
      float *src_c = src_r + ci;
      float *dst_c = dst_r + ci * C8NUM;
      for (size_t i = 0; i < C8NUM; i++) {
        dst_c[i] = src_c[i * col];
      }
    }
    src_r += C8NUM * col;
    dst_r += C8NUM * col;
  }
  for (; ri < row; ri++) {
    for (size_t i = 0; i < col; i++) {
      dst_r[i * C8NUM] = src_r[i];
    }
    src_r += col;
    dst_r += 1;
  }
  return;
}

void RowMajor2Col4Major(float *src_ptr, float *dst_ptr, size_t row, size_t col) {
  size_t row8 = row / C4NUM * C4NUM;
  size_t col4 = col / C4NUM * C4NUM;
  float *src_r = src_ptr;
  float *dst_r = dst_ptr;

  size_t ri = 0;
  for (; ri < row8; ri += C4NUM) {
    size_t ci = 0;
    for (; ci < col4; ci += C4NUM) {
      float *src_c = src_r + ci;
      float *dst_c = dst_r + ci * C4NUM;

      /* 4x4 row-major to col-major */
#ifdef ENABLE_ARM32
      size_t stride = col * 4;
      asm volatile(
        "mov r10, %[src_c]\n"
        "mov r12, %[dst_c]\n"

        "vld1.32 {q0}, [r10], %[stride]\n"
        "vld1.32 {q1}, [r10], %[stride]\n"
        "vld1.32 {q2}, [r10], %[stride]\n"
        "vld1.32 {q3}, [r10], %[stride]\n"

        "vtrn.32 d0, d2\n"
        "vtrn.32 d1, d3\n"
        "vtrn.32 d4, d6\n"
        "vtrn.32 d5, d7\n"

        "vswp d1, d4\n"
        "vswp d3, d6\n"

        "vst1.32 {q0}, [r12]!\n"
        "vst1.32 {q1}, [r12]!\n"
        "vst1.32 {q2}, [r12]!\n"
        "vst1.32 {q3}, [r12]!\n"

        :
        : [ dst_c ] "r"(dst_c), [ src_c ] "r"(src_c), [ stride ] "r"(stride)
        : "r10", "r12", "q0", "q1", "q2", "q3");
#else
      for (int tr = 0; tr < C4NUM; tr++) {
        for (int tc = 0; tc < C4NUM; tc++) {
          dst_c[tc * C4NUM + tr] = src_c[tr * col + tc];
        }
      }
#endif
    }
    for (; ci < col; ci++) {
      float *src_c = src_r + ci;
      float *dst_c = dst_r + ci * C4NUM;
      for (size_t i = 0; i < C4NUM; i++) {
        dst_c[i] = src_c[i * col];
      }
    }
    src_r += C4NUM * col;
    dst_r += C4NUM * col;
  }
  for (; ri < row; ri++) {
    for (size_t i = 0; i < col; i++) {
      dst_r[i * C4NUM] = src_r[i];
    }
    src_r += col;
    dst_r += 1;
  }
  return;
}

void MatMul12x8(const float *a, const float *b, float *dst, const float *bias, ActType act_type, int deep, int row,
                int col, int stride, int out_type) {
  if (out_type == OutType_Nhwc) {
    for (int r = 0; r < row; r++) {
      for (int c = 0; c < col; c++) {
        int r12div = r / 12, r12mod = r % 12;
        int c8div = c / 8, c8mod = c % 8;
        size_t ci = r * stride + c;
        float value = 0;
        for (int d = 0; d < deep; d++) {
          size_t ai = r12div * deep * 12 + d * 12 + r12mod;
          size_t bi = c8div * deep * 8 + d * 8 + c8mod;
          value = value + a[ai] * b[bi];
        }
        if (bias != NULL) value += bias[c];
        if (act_type == ActType_Relu6) value = MSMIN(6.0f, value);
        if (act_type != ActType_No) value = MSMAX(0.0f, value);
        dst[ci] = value;
      }
    }
  } else if (out_type == OutType_C8) {
    int col_8 = UP_ROUND(col, C8NUM);
    int row_12 = UP_ROUND(row, C12NUM);
    for (int r = 0; r < row_12; r++) {
      for (int c = 0; c < col_8; c++) {
        int r12div = r / C12NUM, r12mod = r % C12NUM;
        int c8div = c / C8NUM, c8mod = c % C8NUM;
        size_t ci = (c8div * C8NUM * row_12 + r * C8NUM + c8mod);
        float value = 0;
        for (int d = 0; d < deep; d++) {
          size_t ai = r12div * deep * C12NUM + d * C12NUM + r12mod;
          size_t bi = c8div * deep * C8NUM + d * C8NUM + c8mod;
          value = value + a[ai] * b[bi];
        }
        if (bias != NULL) value += bias[c];
        if (act_type == ActType_Relu6) value = MSMIN(6.0f, value);
        if (act_type != ActType_No) value = MSMAX(0.0f, value);
        dst[ci] = value;
      }
    }
  } else {
    for (int i = 0; i < row; ++i) {
      int src_r_offset = i;
      int dst_r_offset = i * col * stride;
      for (int j = 0; j < col; ++j) {
        int c8div = j / 8, c8mod = j % 8;
        size_t ci = dst_r_offset + c8div * 8 * stride + c8mod;
        float value = 0;
        for (int d = 0; d < deep; ++d) {
          size_t ai = src_r_offset + d * C12NUM;
          size_t bi = c8div * deep * 8 + d * 8 + c8mod;
          value = value + a[ai] * b[bi];
        }
        if (bias != NULL) value += bias[j];
        if (act_type == ActType_Relu6) value = MSMIN(6.0f, value);
        if (act_type != ActType_No) value = MSMAX(0.0f, value);
        dst[ci] = value;
      }
    }
  }
  return;
}

void MatMul4x8(const float *a, const float *b, float *dst, const float *bias, ActType act_type, int deep, int row,
               int col, int stride, int out_type) {
  if (out_type == OutType_C8) {
    int col_8 = UP_ROUND(col, C8NUM);
    int row_4 = UP_ROUND(row, C4NUM);
    for (int r = 0; r < row_4; r++) {
      for (int c = 0; c < col_8; c++) {
        int r4div = r / C4NUM, r4mod = r % C4NUM;
        int c8div = c / C8NUM, c8mod = c % C8NUM;
        size_t ci = (c8div * C8NUM * row_4 + r * C8NUM + c8mod);
        float value = 0;
        for (int d = 0; d < deep; d++) {
          size_t ai = r4div * deep * C4NUM + d * C4NUM + r4mod;
          size_t bi = c8div * deep * C8NUM + d * C8NUM + c8mod;
          value = value + a[ai] * b[bi];
        }
        if (bias != NULL) value += bias[c];
        if (act_type == ActType_Relu6) value = MSMIN(6.0f, value);
        if (act_type != ActType_No) value = MSMAX(0.0f, value);
        dst[ci] = value;
      }
    }
  }
  return;
}

void MatMulOpt(const float *a, const float *b, float *c, const float *bias, ActType act_type, int deep, int row,
               int col, size_t stride, int out_type) {
#ifdef ENABLE_ARM64
  if (out_type == 2 && row <= 8) {
    MatmulFloatNeon64OptRemain(a, b, c, deep, row, col, stride);
  } else {
    MatmulFloatNeon64Opt(a, b, c, bias, (int)act_type, deep, row, col, stride, (int)(out_type == OutType_Nhwc),
                         (int)(out_type == OutType_TileC8));
  }
#elif ENABLE_ARM32
  MatmulFloatNeon32Opt(a, b, c, bias, (int)act_type, deep, row, col, stride, (int)(out_type == OutType_Nhwc),
                       (int)(out_type == OutType_TileC8));
#else
  MatMul12x8(a, b, c, bias, act_type, deep, row, col, stride, out_type);
#endif
}

#ifdef ENABLE_NNACL_INFER_SHAPE
static void SwapDims(int *dims, int index1, int index2) {
  int tmp = dims[index1];
  dims[index1] = dims[index2];
  dims[index2] = tmp;
}

int MatMulInferShape(int **in_shape, int in_num, size_t *dim_size, int *out_shape, int *in_format,
                     int *out_format, int *in_datatype, int *out_datatype, OpParameter *param) {
  *out_datatype = in_datatype[0];
  *out_format = in_format[0];
  if (dim_size[0] < 2 || dim_size[1] < 2) {
    return NNACL_PARAM_INVALID;
  }

  for (int i = 0; i < dim_size[0] - 2; ++i) {
    if (in_shape[0][i] != in_shape[1][i]) {
      return NNACL_PARAM_INVALID;
    }
  }
  MatMulParameter *matmul_param = (MatMulParameter *)param;
  if (matmul_param->a_transpose_) {
    SwapDims(in_shape[0], dim_size[0] - 1, dim_size[0] - 2);
  }
  if (matmul_param->b_transpose_) {
    SwapDims(in_shape[1], dim_size[1] - 1, dim_size[1] - 2);
  }
  for (int i = 0; i < dim_size[0] - 1; ++i) {
    out_shape[i] = in_shape[0][i];
  }
  out_shape[dim_size[0] - 1] = in_shape[1][dim_size[1] - 1];
  return NNACL_OK;
}
#endif
