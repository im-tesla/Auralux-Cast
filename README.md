# 🎧 Auralux Cast

Auralux Cast is a **Windows-only audio caster** that captures system audio via **WASAPI loopback** and streams it using the **SRT protocol**. Whether you're building a multi-room audio setup or just want to overengineer the hell out of sending sound to your kitchen, Auralux Cast has you covered.

> ⚠️ Warning: This is raw PCM over SRT. Bring your own decoder. Have a look into example :)

## 🔥 Features

- 🎯 Capture any system audio output via WASAPI (loopback)
- 🚀 Send it over **SRT** (Secure Reliable Transport)
- 🧰 CLI for power users and script nerds
- 🖥️ GUI version (sleek, modern, not a WinForms disaster) in the `gui/` folder or [Releases](https://github.com/im-tesla/auralux-cast/releases).
- 🧪 Works perfectly with `ffplay`, `ffmpeg`, and other SRT clients
- 🛠️ Audio format: `f32le`, stereo, 48kHz

---

## 📦 Installation

1. Download from the [Releases](https://github.com/im-tesla/auralux-cast/releases).
2. Extract somewhere that won't get deleted by accident (like your soul).
3. Run `auracast.exe` from the CLI, or open the GUI if you fear terminals.

---

## 🧑‍💻 CLI Usage

    auracast.exe <ip> <port> [device_index]

### Options

| Flag     | Description                            |
|----------|----------------------------------------|
| `-list`  | List all available audio output devices |
| `-help`  | Show help message                      |

### Examples

    auracast.exe 127.0.0.1 1234
    # Uses default device

    auracast.exe 10.0.0.167 1234 1
    # Uses device index 1

    auracast.exe -list
    # Prints all audio devices

---

## 📥 Receiving the Stream

You can use **ffplay** to receive and decode the stream. Here’s a working example:

    ffplay -fflags nobuffer -f f32le -ar 48000 -ac 2 \
      -i "srt://0.0.0.0:1234?mode=listener&transtype=live"

- `-f f32le`: Raw float 32-bit little-endian PCM
- `-ar 48000`: 48kHz sample rate
- `-ac 2`: Stereo audio
- SRT is in listener mode, just waiting for Auralux Cast to call

---

## 🖼 GUI Version

Yes, there's a GUI. Yes, it's not hideous.

- Found in the `gui/` folder or bundled in the release
- Built using modern frameworks (none of that MFC fossil stuff)
- Select device, enter IP and port, press cast

---

## 🧠 Internals (a.k.a. for nerds)

- Uses **WASAPI loopback** for device capture
- Uses **SRT in message mode**, capped to 1316 bytes per packet
- Breaks audio data into chunks and sends over SRT
- Handles silence flags and escape-key termination (you're welcome)
- Multithreaded: Audio loop runs tight with ~10ms polling

---

## 👻 Known Limitations

- Windows only (if you're on Linux... go touch some grass or write your own)
- Raw audio only — no compression, no encoding
- Stream must be interpreted correctly on the receiver side (use `ffplay` unless you hate yourself)

---

## 🛠 Dependencies

- SRT library
- Windows Core Audio APIs (WASAPI)
- A brain (optional but helpful)

---

## 🧪 Future Ideas

- Add Opus/FLAC compression (if latency isn't your god)
- Auto device switching when default changes
- Built-in `ffmpeg` pipe integration

---

## 🧙‍♂️ Credits

- Written by a sleep-deprived wizard who clearly didn't want to use VB
- Respect to the **SRT Alliance** for the protocol
- Inspired by the eternal pain of getting audio across a LAN properly

---

## 🤝 License

MIT — Do whatever you want, just don’t blame me when it crashes during your wedding livestream.
