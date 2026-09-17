# ⚡ P10 IoT Sleek Matrix Controller (ESP8266 + P10 Matrix)

Website antarmuka pengatur Jam Digital & Running Text untuk Panel **P10 LED Matrix (HUB12 / DMD)** bertenaga **ESP8266** dengan arsitektur **Sidebar Fixed (Desktop)**, **Fixed Top Navigation (Mobile)**, dan desain **Cyber Purple & Neon Aqua** yang ultra-ringkas, responsif, dan modern.

---

## 🌟 Fitur & Pembaruan Desain

1. **🖥️ Layout Desktop dengan Fixed Sidebar**:
   - Sidebar navigasi tetap di posisi kiri (*fixed*) sehingga tidak ikut tergulir saat halaman di-scroll.
   - 3 Tab Navigasi Utama: **Dashboard**, **Perangkat**, dan **Panduan**.
   - Dilengkapi indikator status koneksi ESP8266 & tombol *Quick Sync Waktu HP*.

2. **📱 Layout Mobile Responsif (Fixed Top Nav)**:
   - Sidebar secara otomatis berpindah ke atas (*fixed top navigation bar*) dengan efek blur kaca transparan.
   - Kartu kontrol tersusun rapi dalam **1 kolom** vertikal untuk kenyamanan navigasi satu tangan.

3. **🎴 Dashboard Bersih & Fokus (Tepat 5 Card)**:
   - **Card 1: Simulator Realtime P10** (Visualisasi Dot-Matrix LED, pilihan warna Merah/Cyan/Hijau/Amber/Ungu, mode jam/teks).
   - **Card 2: Pesan Running Text** (Input tunggal pesan utama dengan tombol hapus cepat dan chip template instan).
   - **Card 3: Efek & Mode Animasi** (Scroll Kiri, Scroll Kanan, Diam di Tengah, dan Kedip di Tengah + Slider Kecepatan).
   - **Card 4: Durasi Tampilan** (Slider Durasi Jam 3-60s, Durasi Teks 3-60s, dan tombol Sinkronisasi Waktu Instan).
   - **Card 5: Kecerahan & Mode Malam** (Slider Kecerahan LED 5-100% dan sakelar Redup Otomatis saat malam).

4. **🎯 Teks Animasi Presisi di Tengah (Blink & Static)**:
   - Teks running text pada mode **Kedip (Blink)** dan **Diam (Static)** berada presisi di **tengah layar simulator (center)** secara horizontal dan vertikal.

5. **🎨 Color System Cyber Purple & Neon Aqua**:
   - **Base Color (Ungu)**: Deep Amethyst / Obsidian Violet (`#0b0816` & `#140f26`) yang mewah, elegan, dan nyaman di mata.
   - **Primary Accent**: Electric Violet / Purple (`#a855f7` & `#c084fc`) untuk pendaran ikon, border, dan tombol utama.
   - **Secondary Accent**: Electric Aqua / Neon Cyan (`#00f0ff`) untuk active states, slider highlights, dan badge nilai.

---

## 📂 Struktur File

```
E:/Web IOT/
├── index.html              # Halaman antarmuka Fixed Sidebar & 5 Cards Ringkas
├── style.css               # Desain Cyber Purple & Neon Aqua dengan transisi halus
├── app.js                  # Logika simulator, tab switcher, RTC timer, dan REST API
├── arduino_p10_esp8266.ino # Sketch program Arduino IDE untuk ESP8266 NodeMCU
└── README.md               # Dokumentasi lengkap proyek
```

---

## 🛠️ Panduan Pin ESP8266 ke Panel P10 (HUB12)

| Pin ESP8266 (NodeMCU) | Pin Panel P10 (HUB12) | Keterangan |
| :--- | :--- | :--- |
| **D0 (GPIO16)** | **A** | Address Line A |
| **D6 (GPIO12)** | **B** | Address Line B |
| **D5 (GPIO14)** | **CLK** | Clock Pin |
| **D7 (GPIO13)** | **R (DATA)** | Red Data / Data In |
| **D8 (GPIO15)** | **OE / NOE** | Output Enable (PWM Kecerahan) |
| **D4 (GPIO2)** | **LAT / SCLK** | Strobe / Latch Pin |
| **GND** | **GND** | Ground bersama |
| *Power Supply 5V (3A - 5A)* | **VCC & GND** | Hubungkan langsung ke terminal power P10 |

---

## 🚀 Cara Menjalankan

### 1. Uji Coba Langsung di Browser (PC / Laptop / HP):
- Buka file [`index.html`](file:///E:/Web%20IOT/index.html) di browser (Chrome, Edge, Safari, Firefox).
- Navigasi antar menu **Dashboard**, **Perangkat**, dan **Panduan**.
- Uji pengubahan teks, kecepatan gulir, dan sinkronisasi waktu.

### 2. Upload ke ESP8266 (LittleFS):
1. Buat folder bernama `data` di dalam folder sketch Arduino Anda.
2. Salin file [`index.html`](file:///E:/Web%20IOT/index.html), [`style.css`](file:///E:/Web%20IOT/style.css), dan [`app.js`](file:///E:/Web%20IOT/app.js) ke dalam folder `data/`.
3. Gunakan plugin **ESP8266 LittleFS Data Upload** di Arduino IDE untuk mengunggah file web ke memori flash ESP8266.
4. Sambungkan HP ke Wi-Fi AP: `P10_CLOCK_IOT` (Password: `12345678`).
5. Buka browser dan akses `http://192.168.4.1`.
