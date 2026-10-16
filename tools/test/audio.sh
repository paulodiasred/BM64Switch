#!/bin/bash
export SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=/tmp/audio.raw
source "$(dirname "$0")/common.sh"
timeout 40 "$BIN" > game.log 2>&1
python3 - <<'PY'
import struct, math
data = open('/tmp/audio.raw','rb').read()
n = len(data)//4
print("bytes", len(data), "seconds@48k stereo f32", n/2/48000)
vals = struct.unpack('<%df' % n, data[:n*4])
for sec in range(0, int(n/2/48000), 4):
    chunk = vals[sec*96000:(sec+1)*96000]
    if not chunk: break
    rms = math.sqrt(sum(v*v for v in chunk)/len(chunk))
    print("t=%2ds rms=%.4f peak=%.3f" % (sec, rms, max(abs(v) for v in chunk)))
PY
python3 - <<"PY2"
import struct, wave
d = open("/tmp/audio.raw","rb").read()
n = len(d)//4
v = struct.unpack("<%df" % n, d[:n*4])[48000*2*14:48000*2*34]
w = wave.open("audio_sample.wav","wb"); w.setnchannels(2); w.setsampwidth(2); w.setframerate(48000)
w.writeframes(b"".join(struct.pack("<h", max(-32768, min(32767, int(x*32767)))) for x in v)); w.close()
print("wav ok")
PY2
