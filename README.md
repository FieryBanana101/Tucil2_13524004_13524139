# Tugas Kecil 2 IF2211 Strategi Algoritma
## Voxelization Objek 3D menggunakan Octree

- NIM Anggota: 13524004, 13524139
- Nama: Muhammad Fatih Irkham Mauludi, Azri Arzaq Pohan
- Kelas: K1 & K3

Program ini mengkonversi model 3D (file `.obj`) menjadi model *voxel* (tersusun dari kubus-kubus kecil) menggunakan struktur data **Octree** dengan algoritma **Divide and Conquer**.

Program membagi ruang 3D menjadi 8 sub-kubus secara rekursif hingga kedalaman maksimum yang ditentukan. Setiap sub-kubus yang berpotongan dengan permukaan model asli akan menjadi sebuah voxel pada hasil akhir.

### Fitur
- Konversi file `.obj` menjadi model voxel `.obj`
- Algoritma Divide and Conquer berbasis Octree
- **Concurrency** — mendukung multi-threading untuk mempercepat proses voxelisasi
- **3D Viewer** — visualisasi interaktif hasil voxelisasi (drag untuk rotasi, scroll untuk zoom)
- Cross-platform (Windows & Linux)
- Statistik lengkap: jumlah voxel, vertex, face, distribusi node per kedalaman, dan waktu proses

## Requirement

| Komponen | Keterangan |
|---|---|
| **Compiler** | `g++` dengan dukungan C++17 (`-std=c++17`) |
| **Build tool** | `make` (GNU Make) |
| **SFML** | SFML 2.x atau SFML 3.x (untuk fitur 3D Viewer) |
| **OS** | Windows atau Linux |

### Instalasi SFML

**Windows (MSYS2/MinGW):**
```bash
pacman -S mingw-w64-x86_64-sfml
```

**Ubuntu/Debian (SFML 2.x):**
```bash
sudo apt install libsfml-dev
```

**Build dari source (SFML 3.x):**
```bash
git clone --branch 3.0.0 https://github.com/SFML/SFML.git
cd SFML && mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=OFF
make -j$(nproc) && sudo make install
```

## Cara Kompilasi

```bash
# Kompilasi standar (dynamic linking)
make

# Kompilasi dengan static linking (untuk SFML 3.x di Linux)
make STATIC=1

# Membersihkan hasil kompilasi
make clean
```

Hasil kompilasi berupa executable di folder `bin/`.

## Cara Menjalankan

```bash
# Jalankan langsung
make run

# Atau jalankan executable
./bin/main        # Linux
bin\main.exe      # Windows
```

Input yang diminta program:

1. **Path file `.obj` input** — path ke file model 3D sumber
2. **Path file `.obj` output** — path untuk menyimpan hasil voxelisasi
3. **Kedalaman Octree (1-12)** — semakin besar, semakin detail hasilnya
4. **Jumlah thread** — menentukan jumlah thread untuk concurrency (0 = maksimum)
5. **Tampilkan 3D Viewer (y/n)** — membuka viewer interaktif setelah proses selesai
6. **Metode sinkronisasi thread** — 0 untuk Spinlock, 1 untuk Sleep
7. **Minimize file size (y/n)** — mengoptimalkan ukuran file output

### Contoh
```
Input .obj file path: test/bunny.obj
Output .obj file path: test/bunny_voxel.obj
Max octree depth (1-12): 6
Number of threads (1-8, 0 for max): 0
Show 3D viewer after voxelization? (y/n): y
Thread sync method (0 for Spinlock, 1 for Sleep): 1
Minimize file size? (y/n): n
```

### Kontrol 3D Viewer
| Input | Aksi |
|---|---|
| Drag kiri mouse | Rotasi kamera |
| Scroll mouse | Zoom in / out |
