/*
 * Softmax Action Selection Tabanli Yuk Dengeleyici
 * Derleme: gcc -o load_balancer main.c -lm
 */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define K 5
#define N 10000
#define ALPHA 0.1
#define TAU 0.5
#define DRIFT_PERIOD 2000
#define CSV_FILENAME "results.csv"

static double rand_normal(void) {
  double u1 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
  double u2 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
  return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static double rand_uniform(double min, double max) {
  return min + ((double)rand() / (double)RAND_MAX) * (max - min);
}

typedef struct {
  double base_latency, noise_stddev, current_latency;
} Server;

static void servers_init(Server servers[K]) {
  double base[] = {20.0, 35.0, 50.0, 15.0, 45.0};
  double noise[] = {5.0, 8.0, 10.0, 3.0, 12.0};
  for (int i = 0; i < K; i++) {
    servers[i].base_latency = base[i];
    servers[i].noise_stddev = noise[i];
    servers[i].current_latency = base[i];
  }
}

/* Drift hesapla ve dizi olarak dondur (yazdirmadan) */
static void calc_drift(double drift_vals[K]) {
  for (int i = 0; i < K; i++)
    drift_vals[i] = rand_uniform(-15.0, 15.0);
}

/* Onceden hesaplanmis drift degerlerini uygula */
static void apply_drift(Server servers[K], const double drift_vals[K]) {
  for (int i = 0; i < K; i++) {
    servers[i].current_latency += drift_vals[i];
    if (servers[i].current_latency < 5.0)
      servers[i].current_latency = 5.0;
    if (servers[i].current_latency > 100.0)
      servers[i].current_latency = 100.0;
  }
}

static double server_get_latency(Server *s) {
  double lat = s->current_latency + s->noise_stddev * rand_normal();
  return (lat < 1.0) ? 1.0 : lat;
}

/* Round-Robin */
typedef struct {
  int next;
} RRState;
static int rr_select(RRState *s) {
  int sel = s->next;
  s->next = (s->next + 1) % K;
  return sel;
}

/* Softmax Action Selection */
typedef struct {
  double Q[K], tau, alpha;
} SoftmaxState;

static int softmax_select(SoftmaxState *s) {
  double probs[K], max_q = -DBL_MAX, sum = 0.0;
  for (int i = 0; i < K; i++)
    if (s->Q[i] > max_q)
      max_q = s->Q[i];
  for (int i = 0; i < K; i++) {
    probs[i] = exp((s->Q[i] - max_q) / s->tau);
    sum += probs[i];
  }
  for (int i = 0; i < K; i++)
    probs[i] /= sum;

  double r = (double)rand() / (double)RAND_MAX, cum = 0.0;
  for (int i = 0; i < K; i++) {
    cum += probs[i];
    if (r <= cum)
      return i;
  }
  return K - 1;
}

static void softmax_update(SoftmaxState *s, int srv, double lat) {
  s->Q[srv] += s->alpha * (-lat - s->Q[srv]);
}

/* Istatistikler */
typedef struct {
  double total_lat, sum_sq;
  int count, srv_counts[K];
} Stats;

static void stats_record(Stats *s, int srv, double lat) {
  s->total_lat += lat;
  s->sum_sq += lat * lat;
  s->count++;
  s->srv_counts[srv]++;
}
static double stats_mean(const Stats *s) {
  return s->count > 0 ? s->total_lat / s->count : 0.0;
}
static double stats_stddev(const Stats *s) {
  if (s->count < 2)
    return 0.0;
  double m = stats_mean(s), v = (s->sum_sq / s->count) - (m * m);
  return v > 0.0 ? sqrt(v) : 0.0;
}

static void run_simulation(void) {
  Server srv_rr[K], srv_rand[K], srv_sm[K];
  servers_init(srv_rr);
  servers_init(srv_rand);
  servers_init(srv_sm);

  RRState rr = {0};
  SoftmaxState sm = {.tau = TAU, .alpha = ALPHA};
  Stats st_rr = {0}, st_rand = {0}, st_sm = {0};

  FILE *csv = fopen(CSV_FILENAME, "w");
  if (!csv) {
    fprintf(stderr, "HATA: CSV acilamadi!\n");
    return;
  }
  fprintf(csv, "Adim,RR,Random,Softmax,RR_Ort,Random_Ort,Softmax_Ort\n");

  printf(
      "\n================================================================\n");
  printf("  SIMULASYON: K=%d, N=%d, alpha=%.2f, tau=%.2f, drift=%d\n", K, N,
         ALPHA, TAU, DRIFT_PERIOD);
  printf(
      "================================================================\n\n");

  for (int step = 0; step < N; step++) {
    /* Drift: tek seferde hesapla, uc sunucu setine ayni degerleri uygula */
    if (step > 0 && step % DRIFT_PERIOD == 0) {
      double dv[K];
      srand((unsigned int)(step * 12345 + 67890));
      calc_drift(dv);
      apply_drift(srv_rr, dv);
      apply_drift(srv_rand, dv);
      apply_drift(srv_sm, dv);
      printf("\n  [DRIFT] Adim %d:\n", step);
      for (int i = 0; i < K; i++)
        printf("    Sunucu %d: gecikme = %.1f ms\n", i,
               srv_sm[i].current_latency);
      printf("\n");
    }
    srand((unsigned int)(time(NULL) + step));

    int s1 = rr_select(&rr);
    double l1 = server_get_latency(&srv_rr[s1]);
    stats_record(&st_rr, s1, l1);

    int s2 = rand() % K;
    double l2 = server_get_latency(&srv_rand[s2]);
    stats_record(&st_rand, s2, l2);

    int s3 = softmax_select(&sm);
    double l3 = server_get_latency(&srv_sm[s3]);
    softmax_update(&sm, s3, l3);
    stats_record(&st_sm, s3, l3);

    fprintf(csv, "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", step, l1, l2, l3,
            stats_mean(&st_rr), stats_mean(&st_rand), stats_mean(&st_sm));

    if ((step + 1) % 2000 == 0)
      printf("  Adim %5d/%d | RR: %6.2f | Random: %6.2f | Softmax: %6.2f ms\n",
             step + 1, N, stats_mean(&st_rr), stats_mean(&st_rand),
             stats_mean(&st_sm));
  }
  fclose(csv);

  /* Sonuc */
  printf(
      "\n================================================================\n");
  printf("                    SONUC ANALIZI\n");
  printf(
      "================================================================\n\n");
  printf("  %-18s %10s %10s %10s\n", "Metrik", "RR", "Random", "Softmax");
  printf("  %-18s %10s %10s %10s\n", "------------------", "----------",
         "----------", "----------");
  printf("  %-18s %10.2f %10.2f %10.2f\n", "Ort. Gecikme(ms)",
         stats_mean(&st_rr), stats_mean(&st_rand), stats_mean(&st_sm));
  printf("  %-18s %10.2f %10.2f %10.2f\n", "Std. Sapma(ms)",
         stats_stddev(&st_rr), stats_stddev(&st_rand), stats_stddev(&st_sm));
  printf("  %-18s %10.2f %10.2f %10.2f\n", "Toplam(ms)", st_rr.total_lat,
         st_rand.total_lat, st_sm.total_lat);

  double imp_rr =
      ((stats_mean(&st_rr) - stats_mean(&st_sm)) / stats_mean(&st_rr)) * 100.0;
  double imp_rand =
      ((stats_mean(&st_rand) - stats_mean(&st_sm)) / stats_mean(&st_rand)) *
      100.0;
  printf("\n  Softmax Iyilestirme: RR'ye gore %%%.1f | Random'a gore %%%.1f\n",
         imp_rr, imp_rand);

  printf("\n  Istek Dagilimi:\n  %-8s", "Sunucu");
  for (int i = 0; i < K; i++)
    printf(" %6d", i);
  printf("\n  %-8s", "RR");
  for (int i = 0; i < K; i++)
    printf(" %6d", st_rr.srv_counts[i]);
  printf("\n  %-8s", "Random");
  for (int i = 0; i < K; i++)
    printf(" %6d", st_rand.srv_counts[i]);
  printf("\n  %-8s", "Softmax");
  for (int i = 0; i < K; i++)
    printf(" %6d", st_sm.srv_counts[i]);

  printf("\n\n  Softmax Q Degerleri:\n");
  for (int i = 0; i < K; i++)
    printf("    Sunucu %d: Q=%.4f (gecikme: %.2f ms)\n", i, sm.Q[i], -sm.Q[i]);

  printf("\n  CSV: '%s'\n", CSV_FILENAME);
  printf(
      "================================================================\n\n");
}

int main(void) {
  srand((unsigned int)time(NULL));
  printf(
      "\n================================================================\n");
  printf("   SOFTMAX TABANLI YUK DENGELEYICI\n");
  printf("================================================================\n");
  run_simulation();
  return 0;
}
