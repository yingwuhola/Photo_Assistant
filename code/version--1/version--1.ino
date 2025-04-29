/* Edge Impulse ingestion SDK
* Copyright (c) 2022 EdgeImpulse Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

// 如果目标设备内存有限，可以移除此宏以节省约10K内存
#define EIDSP_QUANTIZE_FILTERBANK   0

/**
* 定义模型窗口中的切片数。例如，一个1000毫秒的模型窗口，
* 如果切片数设置为4，则每个切片的大小为250毫秒。
* 更多信息请参阅：https://docs.edgeimpulse.com/docs/continuous-audio-sampling 
*/
#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 4

/* 包含必要的头文件 ---------------------------------------------------------------- */
#include <PDM.h>
#include <photo_assist_inferencing.h>

/** 音频缓冲区、指针和选择器 */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
static bool record_ready = false;
static signed short *sampleBuffer;
static bool debug_nn = false; // 将其设置为true可以查看从原始信号生成的特征
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);

// 添加LED控制变量
bool led_on = false;

void setup() {
    Serial.begin(115200);
    // 等待USB连接
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo");

    ei_printf("推理设置:\n");
    ei_printf("\t间隔: %.2f 毫秒。\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\t帧大小: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\t采样长度: %d 毫秒。\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\t类别数量: %d\n", sizeof(ei_classifier_inferencing_categories) /
                                            sizeof(ei_classifier_inferencing_categories[0]));

    run_classifier_init();
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: 无法分配音频缓冲区（大小 %d），这可能是由于模型的窗口长度导致的\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }

    // 设置LED引脚为输出模式
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
}

void loop() {
    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: 音频录制失败...\n");
        return;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: 推理失败 (%d)\n", r);
        return;
    }

    if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
        // 打印预测结果
        ei_printf("预测结果 ");
        ei_printf("(DSP: %d 毫秒，分类: %d 毫秒，异常: %d 毫秒)",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);
        ei_printf(": \n");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            ei_printf("    %s: %.5f\n", result.classification[ix].label,
                      result.classification[ix].value);
        }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    异常分数: %.3f\n", result.anomaly);
#endif

        print_results = 0;

        // 检查是否识别到关键词"picture"
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            if (strcmp(result.classification[ix].label, "picture") == 0 && result.classification[ix].value > 0.5) {
                if (!led_on) {
                    // 如果识别到"picture"且置信度大于0.5，点亮红色LED
                    digitalWrite(LEDR, LOW);
                    digitalWrite(LEDG, HIGH);
                    digitalWrite(LEDB, HIGH);
                    led_on = true;
                    ei_printf("LED turned on!\n");
                }
                break;
            } else {
                // 如果未识别到"picture"或置信度低于0.5，熄灭LED
                if (led_on) {
                    digitalWrite(LEDR, HIGH);
                    digitalWrite(LEDG, HIGH);
                    digitalWrite(LEDB, HIGH);
                    led_on = false;
                    ei_printf("LED turned off!\n");
                }
            }
        }
    }
}

static void pdm_data_ready_inference_callback(void) {
    int bytesAvailable = PDM.available();

    int bytesRead = PDM.read((char *)&sampleBuffer[0], bytesAvailable);

    if (record_ready == true) {
        for (int i = 0; i < bytesRead >> 1; i++) {
            inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

            if (inference.buf_count >= inference.n_samples) {
                inference.buf_select ^= 1;
                inference.buf_count = 0;
                inference.buf_ready = 1;
            }
        }
    }
}

static bool microphone_inference_start(uint32_t n_samples) {
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[1] == NULL) {
        free(inference.buffers[0]);
        return false;
    }

    sampleBuffer = (signed short *)malloc((n_samples >> 1) * sizeof(signed short));

    if (sampleBuffer == NULL) {
        free(inference.buffers[0]);
        free(inference.buffers[1]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    PDM.onReceive(&pdm_data_ready_inference_callback);

    PDM.setBufferSize((n_samples >> 1) * sizeof(int16_t));

    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("PDM 启动失败！");
    }

    PDM.setGain(127);

    record_ready = true;

    return true;
}

static bool microphone_inference_record(void) {
    bool ret = true;

    if (inference.buf_ready == 1) {
        ei_printf(
            "错误：样本缓冲区溢出。请减少模型窗口中的切片数 "
            "(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
        ret = false;
    }

    while (inference.buf_ready == 0) {
        delay(1);
    }

    inference.buf_ready = 0;

    return ret;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

    return 0;
}

static void microphone_inference_end(void) {
    PDM.end();
    free(inference.buffers[0]);
    free(inference.buffers[1]);
    free(sampleBuffer);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "传感器类型不匹配。"
#endif