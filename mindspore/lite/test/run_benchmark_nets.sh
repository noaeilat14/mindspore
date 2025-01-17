#!/bin/bash

# Run converter on x86 platform:
function Run_Converter() {
    # Unzip x86 runtime and convertor
    cd ${x86_path} || exit 1
    tar -zxf mindspore-lite-${version}-runtime-x86-${process_unit_x86}.tar.gz || exit 1

    tar -zxf mindspore-lite-${version}-converter-ubuntu.tar.gz || exit 1
    cd ${x86_path}/mindspore-lite-${version}-converter-ubuntu || exit 1
    cp converter/converter_lite ./ || exit 1
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib/:./third_party/protobuf/lib:./third_party/flatbuffers/lib

    # Convert the models
    cd ${x86_path}/mindspore-lite-${version}-converter-ubuntu || exit 1

    rm -rf ${ms_models_path}
    mkdir -p ${ms_models_path}

    # Convert tflite models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_converter_log_file}"
        echo './converter_lite  --fmk=TFLITE --modelFile='${models_path}'/'${model_name}' --outputFile='${ms_models_path}'/'${model_name}'' >> "${run_converter_log_file}"
        ./converter_lite  --fmk=TFLITE --modelFile=$models_path/${model_name} --outputFile=${ms_models_path}/${model_name}
        if [ $? = 0 ]; then
            converter_result='converter tflite '${model_name}' pass';echo ${converter_result} >> ${run_converter_result_file}
        else
            converter_result='converter tflite '${model_name}' failed';echo ${converter_result} >> ${run_converter_result_file};return 1
        fi
    done < ${models_tflite_config}

    # Convert caffe models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_converter_log_file}"
        echo './converter_lite  --fmk=CAFFE --modelFile='${models_path}'/'${model_name}'.prototxt --weightFile='${models_path}'/'${model_name}'.caffemodel --outputFile='${ms_models_path}'/'${model_name}'' >> "${run_converter_log_file}"
        ./converter_lite  --fmk=CAFFE --modelFile=${models_path}/${model_name}.prototxt --weightFile=${models_path}/${model_name}.caffemodel --outputFile=${ms_models_path}/${model_name}
        if [ $? = 0 ]; then
            converter_result='converter caffe '${model_name}' pass';echo ${converter_result} >> ${run_converter_result_file}
        else
            converter_result='converter caffe '${model_name}' failed';echo ${converter_result} >> ${run_converter_result_file};return 1
        fi
    done < ${models_caffe_config}

    # Convert onnx models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_converter_log_file}"
        echo './converter_lite  --fmk=ONNX --modelFile='${models_path}'/'${model_name}' --outputFile='${ms_models_path}'/'${model_name}'' >> "${run_converter_log_file}"
        ./converter_lite  --fmk=ONNX --modelFile=${models_path}/${model_name} --outputFile=${ms_models_path}/${model_name}
        if [ $? = 0 ]; then
            converter_result='converter onnx '${model_name}' pass';echo ${converter_result} >> ${run_converter_result_file}
        else
            converter_result='converter onnx '${model_name}' failed';echo ${converter_result} >> ${run_converter_result_file}
        fi
    done < ${models_onnx_config}

    # Convert mindspore models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_converter_log_file}"
        echo './converter_lite  --fmk=MINDIR --modelFile='${models_path}'/'${model_name}' --outputFile='${ms_models_path}'/'${model_name}'' >> "${run_converter_log_file}"
        ./converter_lite  --fmk=MINDIR --modelFile=${models_path}/${model_name} --outputFile=${ms_models_path}/${model_name}
        if [ $? = 0 ]; then
            converter_result='converter mindspore '${model_name}' pass';echo ${converter_result} >> ${run_converter_result_file}
        else
            converter_result='converter mindspore '${model_name}' failed';echo ${converter_result} >> ${run_converter_result_file};return 1
        fi
    done < ${models_mindspore_config}

    # Convert TFLite PostTraining models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_converter_log_file}"
        echo 'convert mode name: '${model_name}' begin.'
        echo './converter_lite  --fmk=TFLITE --modelFile='${models_path}'/'${model_name}' --outputFile='${ms_models_path}'/'${model_name}_posttraining' --quantType=PostTraining --config_file='${models_path}'/'${model_name}'_posttraining.config' >> "${run_converter_log_file}"
        ./converter_lite  --fmk=TFLITE --modelFile=$models_path/${model_name} --outputFile=${ms_models_path}/${model_name}_posttraining --quantType=PostTraining --config_file=${models_path}/${model_name}_posttraining.config
        if [ $? = 0 ]; then
            converter_result='converter post_training '${model_name}' pass';echo ${converter_result} >> ${run_converter_result_file}
        else
            converter_result='converter post_training '${model_name}' failed';echo ${converter_result} >> ${run_converter_result_file};return 1
        fi
    done < ${models_tflite_posttraining_config}

    # Convert TFLite AwareTraining models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_converter_log_file}"
        echo './converter_lite  --fmk=TFLITE --modelFile='${models_path}'/'${model_name}' --outputFile='${ms_models_path}'/'${model_name}' --quantType=AwareTraining' >> "${run_converter_log_file}"
        ./converter_lite  --fmk=TFLITE --modelFile=${models_path}/${model_name} --outputFile=${ms_models_path}/${model_name} --quantType=AwareTraining
        if [ $? = 0 ]; then
            converter_result='converter aware_training '${model_name}' pass';echo ${converter_result} >> ${run_converter_result_file}
        else
            converter_result='converter aware_training '${model_name}' failed';echo ${converter_result} >> ${run_converter_result_file};return 1
        fi
    done < ${models_tflite_awaretraining_config}

    # Copy fp16 ms models:
    while read line; do
        model_name=${line%.*}
        if [[ $model_name == \#* ]]; then
            continue
        fi
        echo 'cp '${ms_models_path}'/'${model_name}'.ms' ${ms_models_path}'/'${model_name}'.fp16.ms'
        cp ${ms_models_path}/${model_name}.ms ${ms_models_path}/${model_name}.fp16.ms
        if [ $? = 0 ]; then
            converter_result='converter fp16 '${model_name}' pass';echo ${converter_result} >> ${run_converter_result_file}
        else
            converter_result='converter fp16 '${model_name}' failed';echo ${converter_result} >> ${run_converter_result_file};return 1
        fi
    done < ${models_fp16_config}

    # Convert weightquant models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_converter_log_file}"
        echo './converter_lite  --fmk=TFLITE --modelFile='${models_path}'/'${model_name}' --outputFile='${ms_models_path}'/'${model_name}'--quantType=WeightQuant --bitNum=8 --quantSize=500 --convWeightQuantChannelThreshold=16' >> "${run_converter_log_file}"
        ./converter_lite  --fmk=TFLITE --modelFile=$models_path/${model_name} --outputFile=${ms_models_path}/${model_name}_weightquant --quantType=WeightQuant --bitNum=8 --quantSize=500 --convWeightQuantChannelThreshold=16
        if [ $? = 0 ]; then
            converter_result='converter weight_quant '${model_name}' pass';echo ${converter_result} >> ${run_converter_result_file}
        else
            converter_result='converter weight_quant '${model_name}' failed';echo ${converter_result} >> ${run_converter_result_file};return 1
        fi
    done < ${models_tflite_weightquant_config}
}

# Run on x86 platform:
function Run_x86() {
    # Run tflite converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_x86_log_file}"
        echo 'cd  '${x86_path}'/mindspore-lite-'${version}'-runtime-x86-'${process_unit_x86} >> "{run_x86_log_file}"
        cd ${x86_path}/mindspore-lite-${version}-runtime-x86-${process_unit_x86} || return 1
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath='${ms_models_path}'/'${model_name}'.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/'${model_name}'.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/'${model_name}'.ms.out' >> "${run_x86_log_file}"
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath=${ms_models_path}/${model_name}.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/${model_name}.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/${model_name}.ms.out >> "${run_x86_log_file}"
        if [ $? = 0 ]; then
            run_result='x86: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='x86: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_tflite_config}

    # Run caffe converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_x86_log_file}"
        echo 'cd  '${x86_path}'/mindspore-lite-'${version}'-runtime-x86-'${process_unit_x86} >> "${run_x86_log_file}"
        cd ${x86_path}/mindspore-lite-${version}-runtime-x86-${process_unit_x86} || return 1
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath='${ms_models_path}'/'${model_name}'.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/'${model_name}'.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/'${model_name}'.ms.out' >> "${run_x86_log_file}"
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath=${ms_models_path}/${model_name}.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/${model_name}.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/${model_name}.ms.out >> "${run_x86_log_file}"
        if [ $? = 0 ]; then
            run_result='x86: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='x86: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_caffe_config}

    # Run onnx converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_x86_log_file}"
        echo 'cd  '${x86_path}'/mindspore-lite-'${version}'-runtime-x86-'${process_unit_x86} >> "${run_x86_log_file}"
        cd ${x86_path}/mindspore-lite-${version}-runtime-x86-${process_unit_x86} || return 1
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath='${ms_models_path}'/'${model_name}'.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/'${model_name}'.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/'${model_name}'.ms.out' >> "${run_x86_log_file}"
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath=${ms_models_path}/${model_name}.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/${model_name}.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/${model_name}.ms.out >> "${run_x86_log_file}"
        if [ $? = 0 ]; then
            run_result='x86: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='x86: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_onnx_config}

    # Run tflite post training quantization converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_x86_log_file}"
        echo 'cd  '${x86_path}'/mindspore-lite-'${version}'-runtime-x86-'${process_unit_x86} >> "${run_x86_log_file}"
        cd ${x86_path}/mindspore-lite-${version}-runtime-x86-${process_unit_x86} || return 1
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath='${ms_models_path}'/'${model_name}'_posttraining.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/quantTraining/mnist_calibration_data/00099.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/'${model_name}'_posttraining.ms.out' >> "${run_x86_log_file}"
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath=${ms_models_path}/${model_name}_posttraining.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/quantTraining/mnist_calibration_data/00099.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/${model_name}_posttraining.ms.out >> "${run_x86_log_file}"
        if [ $? = 0 ]; then
            run_result='x86: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='x86: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_tflite_posttraining_config}

    # Run tflite aware training quantization converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_x86_log_file}"
        echo 'cd  '${x86_path}'/mindspore-lite-'${version}'-runtime-x86-'${process_unit_x86} >> "${run_x86_log_file}"
        cd ${x86_path}/mindspore-lite-${version}-runtime-x86-${process_unit_x86} || return 1
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath='${ms_models_path}'/'${model_name}'.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/${model_name}.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/'${model_name}'.ms.out' >> "${run_x86_log_file}"
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath=${ms_models_path}/${model_name}.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/${model_name}.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/${model_name}.ms.out >> "${run_x86_log_file}"
        if [ $? = 0 ]; then
            run_result='x86: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='x86: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_tflite_awaretraining_config}

    # Run mindspore converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_x86_log_file}"
        echo 'cd  '${x86_path}'/mindspore-lite-'${version}'-runtime-x86-'${process_unit_x86} >> "${run_x86_log_file}"
        cd ${x86_path}/mindspore-lite-${version}-runtime-x86-${process_unit_x86} || return 1
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath='${ms_models_path}'/'${model_name}'.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/'${model_name}'.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/'${model_name}'.ms.out' >> "${run_x86_log_file}"
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath=${ms_models_path}/${model_name}.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/${model_name}.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/${model_name}.ms.out --accuracyThreshold=1.5 >> "${run_x86_log_file}"
        if [ $? = 0 ]; then
            run_result='x86: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='x86: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_mindspore_config}

    # Run tflite weight quantization converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_x86_log_file}"
        echo 'cd  '${x86_path}'/mindspore-lite-'${version}'-runtime-x86-'${process_unit_x86} >> "${run_x86_log_file}"
        cd ${x86_path}/mindspore-lite-${version}-runtime-x86-${process_unit_x86} || return 1
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath='${ms_models_path}'/'${model_name}'.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/'${model_name}'.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/'${model_name}'.ms.out' >> "${run_x86_log_file}"
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib:./third_party/libjpeg-turbo/lib:./third_party/opencv/lib;./benchmark/benchmark --modelPath=${ms_models_path}/${model_name}_weightquant.ms --inDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/input/${model_name}.ms.bin --calibDataPath=/home/workspace/mindspore_dataset/mslite/models/hiai/input_output/output/${model_name}.ms.out >> "${run_x86_log_file}"
        if [ $? = 0 ]; then
            run_result='x86: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='x86: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_tflite_weightquant_config}
}

# Run on arm64 platform:
function Run_arm64() {
    # Unzip arm64
    cd ${arm64_path} || exit 1
    tar -zxf mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}.tar.gz || exit 1

    # If build with minddata, copy the minddata related libs
    cd ${benchmark_test_path} || exit 1
    if [ -f ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/lib/libminddata-lite.so ]; then
        cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/third_party/libjpeg-turbo/lib/libjpeg.so ${benchmark_test_path}/libjpeg.so || exit 1
        cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/third_party/libjpeg-turbo/lib/libturbojpeg.so ${benchmark_test_path}/libturbojpeg.so || exit 1
        cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/third_party/opencv/lib/libopencv_core.so ${benchmark_test_path}/libopencv_core.so || exit 1
        cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/third_party/opencv/lib/libopencv_imgcodecs.so ${benchmark_test_path}/libopencv_imgcodecs.so || exit 1
        cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/third_party/opencv/lib/libopencv_imgproc.so ${benchmark_test_path}/libopencv_imgproc.so || exit 1
        cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/lib/libminddata-lite.so ${benchmark_test_path}/libminddata-lite.so || exit 1
    fi

    cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/lib/libmindspore-lite.so ${benchmark_test_path}/libmindspore-lite.so || exit 1
    cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/lib/libmindspore-lite-fp16.so ${benchmark_test_path}/libmindspore-lite-fp16.so || exit 1
    cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/lib/libmindspore-lite-optimize.so ${benchmark_test_path}/libmindspore-lite-optimize.so || exit 1
    cp -a ${arm64_path}/mindspore-lite-${version}-runtime-arm64-${process_unit_arm64}/benchmark/benchmark ${benchmark_test_path}/benchmark || exit 1

    # adb push all needed files to the phone
    adb -s ${device_id} push ${benchmark_test_path} /data/local/tmp/ > adb_push_log.txt

    # run adb ,run session ,check the result:
    echo 'cd  /data/local/tmp/benchmark_test' > adb_cmd.txt
    echo 'cp  /data/local/tmp/libc++_shared.so ./' >> adb_cmd.txt
    echo 'chmod 777 benchmark' >> adb_cmd.txt

    adb -s ${device_id} shell < adb_cmd.txt

    # Run tflite converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_tflite_config}

    # Run caffe converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_caffe_config}

    # Run onnx converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> "{run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_onnx_config}

    # Run fp16 converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out --fp16Priority=true --accuracyThreshold=5' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out --fp16Priority=true --accuracyThreshold=5' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2 --fp16Priority=true' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2 --fp16Priority=true' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_fp16_config}

    # Run tflite aware training quantization converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_tflite_awaretraining_config}

    # Run gpu tflite converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --device=GPU --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --device=GPU --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64_gpu: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64_gpu: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        #echo ${model_name}
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --device=GPU --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --device=GPU --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64_gpu: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64_gpu: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_tflite_gpu_config}

    # Run GPU fp16 converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --device=GPU --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out --fp16Priority=true --accuracyThreshold=5' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --device=GPU --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out --fp16Priority=true --accuracyThreshold=5' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64_gpu_fp16: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64_gpu_fp16: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --device=GPU --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2 --fp16Priority=true' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --device=GPU --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2 --fp16Priority=true' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64_gpu_fp16: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64_gpu_fp16: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
	#sleep 1
    done < ${models_fp16_gpu_config}

    # Run mindir converted models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out --accuracyThreshold=1.5' >> "${run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out --accuracyThreshold=1.5' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        echo ${model_name} >> "${run_arm64_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> "{run_arm64_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm64_log_file}"
        if [ $? = 0 ]; then
            run_result='arm64: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm64: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_mindspore_config}
}

# Run on arm32 platform:
function Run_arm32() {
    # Unzip arm32
    cd ${arm32_path} || exit 1
    tar -zxf mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}.tar.gz || exit 1

    # If build with minddata, copy the minddata related libs
    cd ${benchmark_test_path} || exit 1
    if [ -f ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/lib/libminddata-lite.so ]; then
        cp -a ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/third_party/libjpeg-turbo/lib/libjpeg.so ${benchmark_test_path}/libjpeg.so || exit 1
        cp -a ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/third_party/libjpeg-turbo/lib/libturbojpeg.so ${benchmark_test_path}/libturbojpeg.so || exit 1
        cp -a ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/third_party/opencv/lib/libopencv_core.so ${benchmark_test_path}/libopencv_core.so || exit 1
        cp -a ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/third_party/opencv/lib/libopencv_imgcodecs.so ${benchmark_test_path}/libopencv_imgcodecs.so || exit 1
        cp -a ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/third_party/opencv/lib/libopencv_imgproc.so ${benchmark_test_path}/libopencv_imgproc.so || exit 1
        cp -a ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/lib/libminddata-lite.so ${benchmark_test_path}/libminddata-lite.so || exit 1
    fi

    cp -a ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/lib/libmindspore-lite.so ${benchmark_test_path}/libmindspore-lite.so || exit 1
    cp -a ${arm32_path}/mindspore-lite-${version}-runtime-arm32-${process_unit_arm32}/benchmark/benchmark ${benchmark_test_path}/benchmark || exit 1

    # adb push all needed files to the phone
    adb -s ${device_id} push ${benchmark_test_path} /data/local/tmp/ > adb_push_log.txt

    # run adb ,run session ,check the result:
    echo 'cd  /data/local/tmp/benchmark_test' > adb_cmd.txt
    echo 'cp  /data/local/tmp/arm32/libc++_shared.so ./' >> adb_cmd.txt
    echo 'chmod 777 benchmark' >> adb_cmd.txt

    adb -s ${device_id} shell < adb_cmd.txt

    # Run fp32 models:
    while read line; do
        model_name=${line}
        if [[ $model_name == \#* ]]; then
          continue
        fi
        echo ${model_name} >> "${run_arm32_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> "${run_arm32_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --inDataPath=/data/local/tmp/input_output/input/'${model_name}'.ms.bin --calibDataPath=/data/local/tmp/input_output/output/'${model_name}'.ms.out' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm32_log_file}"
        if [ $? = 0 ]; then
            run_result='arm32: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm32: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
        # run benchmark test without clib data
        echo ${model_name} >> "${run_arm32_log_file}"
        echo 'cd  /data/local/tmp/benchmark_test' > adb_run_cmd.txt
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> "${run_arm32_log_file}"
        echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/benchmark_test;./benchmark --modelPath='${model_name}'.ms --warmUpLoopCount=1 --loopCount=2' >> adb_run_cmd.txt
        adb -s ${device_id} shell < adb_run_cmd.txt >> "${run_arm32_log_file}"
        if [ $? = 0 ]; then
            run_result='arm32: '${model_name}' pass'; echo ${run_result} >> ${run_benchmark_result_file}
        else
            run_result='arm32: '${model_name}' failed'; echo ${run_result} >> ${run_benchmark_result_file}; return 1
        fi
    done < ${models_arm32_config}
}

# Print start msg before run testcase
function MS_PRINT_TESTCASE_START_MSG() {
    echo ""
    echo -e "-----------------------------------------------------------------------------------------------------------------------------------"
    echo -e "env                  Testcase                                                                                             Result   "
    echo -e "---                  --------                                                                                             ------   "
}

# Print start msg after run testcase
function MS_PRINT_TESTCASE_END_MSG() {
    echo -e "-----------------------------------------------------------------------------------------------------------------------------------"
}

basepath=$(pwd)
echo ${basepath}
#set -e

# Example:sh run_benchmark_nets.sh -a /home/temp_test -c /home/temp_test -r /home/temp_test -m /home/temp_test/models -d "8KE5T19620002408"
while getopts "a:c:r:m:d:" opt; do
    case ${opt} in
        a)
	    arm_path=${OPTARG}
            echo "arm_path is ${OPTARG}"
            ;;
        c)
	    converter_path=${OPTARG}
            echo "converter_path is ${OPTARG}"
            ;;
        r)
	    release_path=${OPTARG}
            echo "release_path is ${OPTARG}"
            ;;
        m)
	    models_path=${OPTARG}
            echo "models_path is ${OPTARG}"
            ;;
        d)
	    device_id=${OPTARG}
            echo "device_id is ${OPTARG}"
            ;;
        ?)
        echo "unknown para"
        exit 1;;
    esac
done

echo ${arm_path}
echo ${converter_path}

mkdir train
arm64_path=${release_path}/android_aarch64
mv ${arm64_path}/*runtime-*train* ./train
file_name=$(ls ${arm64_path}/*runtime-arm64*.tar.gz)
IFS="-" read -r -a file_name_array <<< "$file_name"
version=${file_name_array[2]}
IFS="." read -r -a suffix <<< "${file_name_array[-1]}"
process_unit_arm64=${suffix[0]}

arm32_path=${release_path}/android_aarch32
mv ${arm32_path}/*runtime-*train* ./train
file_name=$(ls ${arm32_path}/*runtime-arm32*.tar.gz)
IFS="-" read -r -a file_name_array <<< "$file_name"
IFS="." read -r -a suffix <<< "${file_name_array[-1]}"
process_unit_arm32=${suffix[0]}

x86_path=${release_path}/ubuntu_x86
file_name=$(ls ${x86_path}/*runtime-x86*.tar.gz)
IFS="-" read -r -a file_name_array <<< "$file_name"
IFS="." read -r -a suffix <<< "${file_name_array[-1]}"
process_unit_x86=${suffix[0]}

# Set models config filepath
models_tflite_config=${basepath}/models_tflite.cfg
models_caffe_config=${basepath}/models_caffe.cfg
models_tflite_awaretraining_config=${basepath}/models_tflite_awaretraining.cfg
models_tflite_posttraining_config=${basepath}/models_tflite_posttraining.cfg
models_tflite_weightquant_config=${basepath}/models_tflite_weightquant.cfg
models_onnx_config=${basepath}/models_onnx.cfg
models_fp16_config=${basepath}/models_fp16.cfg
models_mindspore_config=${basepath}/models_mindspore.cfg
models_tflite_gpu_config=${basepath}/models_tflite_gpu.cfg
models_fp16_gpu_config=${basepath}/models_fp16_gpu.cfg
models_arm32_config=${basepath}/models_arm32.cfg

ms_models_path=${basepath}/ms_models

# Write converter result to temp file
run_converter_log_file=${basepath}/run_converter_log.txt
echo ' ' > ${run_converter_log_file}

run_converter_result_file=${basepath}/run_converter_result.txt
echo ' ' > ${run_converter_result_file}

# Run converter
echo "start Run converter ..."
Run_Converter
Run_converter_PID=$!
sleep 1

wait ${Run_converter_PID}
Run_converter_status=$?

function Print_Converter_Result() {
    MS_PRINT_TESTCASE_END_MSG
    while read line; do
        arr=("${line}")
        printf "%-15s %-20s %-90s %-7s\n" ${arr[0]} ${arr[1]} ${arr[2]} ${arr[3]}
    done < ${run_converter_result_file}
    MS_PRINT_TESTCASE_END_MSG
}

# Check converter result and return value
if [[ ${Run_converter_status} = 0 ]];then
    echo "Run converter success"
    Print_Converter_Result
else
    echo "Run converter failed"
    cat ${run_converter_log_file}
    Print_Converter_Result
    exit 1
fi


# Write benchmark result to temp file
run_benchmark_result_file=${basepath}/run_benchmark_result.txt
echo ' ' > ${run_benchmark_result_file}

run_x86_log_file=${basepath}/run_x86_log.txt
echo 'run x86 logs: ' > ${run_x86_log_file}

run_arm64_log_file=${basepath}/run_arm64_log.txt
echo 'run arm64 logs: ' > ${run_arm64_log_file}

run_arm32_log_file=${basepath}/run_arm32_log.txt
echo 'run arm32 logs: ' > ${run_arm32_log_file}

# Copy the MindSpore models:
echo "Push files to the arm and run benchmark"
benchmark_test_path=${basepath}/benchmark_test
rm -rf ${benchmark_test_path}
mkdir -p ${benchmark_test_path}
cp -a ${ms_models_path}/*.ms ${benchmark_test_path} || exit 1

# Run on x86
echo "start Run x86 ..."
Run_x86 &
Run_x86_PID=$!
sleep 1

# Run on arm64
echo "start Run arm64 ..."
Run_arm64
Run_arm64_status=$?
sleep 1

# Run on arm32
echo "start Run arm32 ..."
Run_arm32
Run_arm32_status=$?
sleep 1

wait ${Run_x86_PID}
Run_x86_status=$?

function Print_Benchmark_Result() {
    MS_PRINT_TESTCASE_START_MSG
    while read line; do
        arr=("${line}")
        printf "%-20s %-100s %-7s\n" ${arr[0]} ${arr[1]} ${arr[2]}
    done < ${run_benchmark_result_file}
    MS_PRINT_TESTCASE_END_MSG
}

# Check benchmark result and return value
if [[ ${Run_x86_status} != 0 ]];then
    echo "Run_x86 failed"
    cat ${run_x86_log_file}
    Print_Benchmark_Result
    exit 1
fi

if [[ ${Run_arm64_status} != 0 ]];then
    echo "Run_arm64 failed"
    cat ${run_arm64_log_file}
    Print_Benchmark_Result
    exit 1
fi

if [[ ${Run_arm32_status} != 0 ]];then
    echo "Run_arm32 failed"
    cat ${run_arm32_log_file}
    Print_Benchmark_Result
    exit 1
fi

echo "Run_x86 and Run_arm64 and Run_arm32 is ended"
Print_Benchmark_Result
exit 0
