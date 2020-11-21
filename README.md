# ESP32-LCDKit

Some sample codes using echo ESP32-LCDKit

### audio

#### 44.1 kHz PCM

`ffmpeg -i input.mp4 -f u16le -acodec pcm_u16le -ar 44100 -ac 1 44100_u16le.pcm`

#### 44.1 kHz MP3

`ffmpeg -i input.mp4 -ar 44100 -ac 1 -q:a 9 44100.mp3`

#### 22.05 kHz MP3

`ffmpeg -i input.mp4 -ar 22050 -ac 1 -q:a 9 22050.mp3`

### video

#### 320x240@15fps

`ffmpeg -i input.mp4 -vf "fps=15,scale=-1:320:flags=lanczos,crop=320:240:(in_w-320)/2:40" -q:v 6 320_15fps.mjpeg`

#### 320x240@30fps

`ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=320:in_h:(in_w-320)/2:0" -q:v 9 320_30fps.mjpeg`

## Sample Video Source

[https://youtu.be/upjTmKXDnFU](https://youtu.be/upjTmKXDnFU){:target="_blank"}
[https://youtu.be/ygNKQzKwXKM](https://youtu.be/ygNKQzKwXKM){:target="_blank"}