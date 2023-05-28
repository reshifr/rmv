# Multi-level vector

## Proof of concept

Program ini adalah PoC untuk rancangan struktur data *array* dinamis yang mampu
mengelola konsumsi memorinya secara "cerdas". *Array* mengonsumsi memori sesuai dengan
jumlah elemen yang tersimpan, sangat kecil *overhead* memori tersisa untuk penggunaan masa depan.
Dapat menyesuaikan ukuran memorinya dalam pertumbuhan linear (100, 102, 104, 106, 108), tidak dalam
eksponensial (100, 200, 400, 800, 1600), sehingga tidak pernah terjadi lompatan memori.

Kelemahannya adalah kompleksitas baca-tulis *array* menjadi `O(log(B, n))`, di mana `B` adalah
ukuran block data terkecil untuk tumbuh dan `n` adalah ukuran data. Beberapa cara dalam pengoptimalan
kompleksitas baca-tulis adalah dengan cara menyimpan pointer ke blok yang sering diakses dengan
mekanisme LRU (*least recently used*) atau LFU (*Least frequently used*).
