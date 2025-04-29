# Photo_Assistant
name: Ying Wu
Edge Impulse projects:https://studio.edgeimpulse.com/studio/680069

# Introduction
## Overview
This project is a voice-controlled photo assistant system based on keyword detection. A deep learning model is trained on the Edge Impulse platform and deployed to an Arduino Nano 33 BLE to recognize specific voice commands. The system’s primary functions are: first, triggering the camera shutter when the keyword “picture” is detected; and second, controlling the rotation direction of a servo motor based on the “left” and “right” commands. To simplify the prototype development, the actions are indicated through the onboard LEDs instead of actual hardware movement — the red LED lights up for taking a picture, the green LED indicates the servo turning left, and the blue LED indicates the servo turning right.
