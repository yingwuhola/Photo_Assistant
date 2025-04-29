# Photo_Assistant
name: Ying Wu

Edge Impulse projects:https://studio.edgeimpulse.com/studio/680069

# Introduction
## Overview
This project is a voice-controlled photo assistant system based on keyword detection. A deep learning model is trained on the Edge Impulse platform and deployed to an Arduino Nano 33 BLE to recognize specific voice commands. The system’s primary functions are: first, triggering the camera shutter when the keyword “picture” is detected; and second, controlling the rotation direction of a servo motor based on the “left” and “right” commands. To simplify the prototype development, the actions are indicated through the onboard LEDs instead of actual hardware movement — the red LED lights up for taking a picture, the green LED indicates the servo turning left, and the blue LED indicates the servo turning right.
## Inspiration
In everyday life, I often find it inconvenient to take photos in certain situations. For example, holding a heavy phone with one hand for an extended period can be tiring, and it can be difficult to reach the shutter button. During group photos, someone usually has to hold the phone, making it hard for them to naturally be part of the picture, or the angle and distance constantly need to be adjusted. These situations can lead to missing many wonderful moments, which can be frustrating. Therefore, I wanted to design a voice-controlled photography device that allows users to take photos with simple voice commands, completely freeing up their hands.
## Research question
How to achieve hands-free photography control through sound recognition based on deep learning methods.

# Application overview
This application can be divided into three main stages:
1. Data Collection:
The built-in PDM microphone of the Arduino Nano 33 BLE Sense Board was used to collect audio data at a sampling rate of 16 kHz. Four labels were defined: picture, left, right, and unknown. Each audio sample was segmented into 1-second windows to standardize the input length for model training.
2. Model Training:
Data preprocessing and model training were performed on the Edge Impulse platform. A one-dimensional convolutional neural network (1D CNN) was used to perform keyword classification. Through multiple rounds of experimentation, various model parameters were tested to identify the most suitable configuration for the task.
3. Deployment:
The trained model was deployed onto the Arduino Nano 33 BLE board for real-time keyword detection. The microcontroller was then programmed to trigger the corresponding LED indicators based on recognized commands, allowing for functional testing and validation of the system’s performance.
