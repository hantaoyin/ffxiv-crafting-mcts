# Simple awk script that extracts the moving average of the self play score from log file.
#
# Usage:
#
# $ ./ffxiv-crafting-mcts | tee log
# $ awk -f <this script> log
BEGIN {
  i = 0
  # Moving average of this many values.
  N = 50
}

/done, score/ {
  n = $6
  p = a[i]
  m1 = m1 + n - p
  m2 = m2 + n * n - p * p
  a[i] = n
  if (i == N) {
    i = 0
  } else {
    i++
  }
  if (NR >= N) {
    avg = m1 / N
    # This standard deviation underestimates due to autocorrelation.
    sd = sqrt((m2 - N * avg * avg) / (N * (N - 1)))
    printf("%e %e\n", avg, sd)
  }
}
