# Softmax Action Selection Tabanlı Yük Dengeleyici

Dağıtık sistemlerde **K adet sunucuya** gelen istekleri, **Softmax Action Selection** algoritması kullanarak dağıtan bir **istemci taraflı yük dengeleyici (client-side load balancer)** uygulaması.

## 🎯 Problem Tanımı

- K adet farklı sunucudan oluşan bir kümeye (cluster) gelen istekler dağıtılmalıdır
- Sunucuların yanıt süreleri **sabit değildir** (non-stationary) ve **gürültülüdür**
- Amaç: Toplam bekleme süresini (latency) minimize etmek

## 🧠 Algoritmalar

### 1. Round-Robin (Baseline)
Sırayla her sunucuya istek gönderir. Sunucu performansını dikkate almaz.

### 2. Random (Baseline)
Uniform rastgele sunucu seçimi yapar. Sunucu performansını dikkate almaz.

### 3. Softmax Action Selection ⭐
Geçmiş performans verisine dayalı olasılıksal bir seçim yapar:

**Seçim Olasılığı (Boltzmann Dağılımı):**
```
P(a) = exp(Q(a) / τ) / Σ exp(Q(j) / τ)
```

**Değer Güncelleme (Incremental Update):**
```
Q(a) ← Q(a) + α × (R - Q(a))
```

- `Q(a)`: Sunucu _a_'nın tahmini değeri
- `τ` (tau): Sıcaklık parametresi (keşif/sömürü dengesi)
- `α`: Öğrenme oranı (non-stationary ortam için sabit)
- `R`: Ödül (= -gecikme)

## ⚙️ Parametreler

| Parametre | Değer | Açıklama |
|-----------|-------|----------|
| K | 5 | Sunucu sayısı |
| N | 10000 | Toplam istek sayısı |
| α (alpha) | 0.1 | Öğrenme oranı |
| τ (tau) | 0.5 | Sıcaklık parametresi |
| Drift Periyodu | 2000 | Performans değişim sıklığı |

## 🏗️ Derleme ve Çalıştırma

### GCC ile (Linux/macOS)
```bash
gcc -Wall -O2 -o load_balancer main.c -lm
./load_balancer
```

### Makefile ile
```bash
make
make run
```

### Windows (MinGW/MSYS2)
```bash
gcc -Wall -O2 -o load_balancer.exe main.c -lm
load_balancer.exe
```

## 📊 Çıktılar

### Konsol Çıktısı
- Simülasyon parametreleri
- Her 2000 adımda ara sonuçlar
- Drift (performans değişimi) bildirimleri
- Karşılaştırmalı sonuç analizi tablosu
- Softmax iyileştirme oranları
- Sunucu başına istek dağılımı

### CSV Dosyası (`results.csv`)
Adım adım gecikme verileri içerir:
- Her adımdaki anlık gecikme (üç algoritma)
- Kümülatif ortalama gecikme (üç algoritma)

## 🔬 Non-Stationary Ortam

Sunucu performansı zamanla değişir:
- **Drift**: Her 2000 istekte sunucuların temel gecikmeleri rastgele değişir
- **Gürültü**: Her istekte Gaussian gürültü eklenir
- Softmax'ın sabit α değeri sayesinde eski gözlemlerin etkisi zamanla azalır

## 📁 Dosya Yapısı

```
softmax-load-balancer/
├── main.c          # Ana kaynak kod (tüm algoritmalar + simülasyon)
├── Makefile        # Derleme dosyası
├── README.md       # Bu dosya
└── results.csv     # Çalıştırma sonrası oluşur
```

## 📖 Kaynak

- Sutton, R.S. & Barto, A.G. (2018). *Reinforcement Learning: An Introduction*. Chapter 2: Multi-Armed Bandits.
