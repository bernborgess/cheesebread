/* MDH WCET BENCHMARK SUITE. File version $Id: fft1.c 7800 2018-01-19 12:43:12Z lkg02 $ */

let PI: Float64 = 3.14159
let M_PI: Float64 = 3.14159

var ar: Array<Float64> = Array<Float64>(8, repeat: 0.0)
var ai: Array<Float64> = Array<Float64>(8, repeat: 0.0)

func fabs(n: Float64): Float64 {
  var f: Float64 = 0.0
  if (n >= 0.0) {
    f = n
  } else {
    f = -n
  }
  return f
}

func log(n: Float64): Float64 {
  return 4.5
}

func sin(radIn: Float64): Float64 {
  var rad: Float64 = radIn
  var app: Float64 = 0.0
  var diff: Float64 = 0.0
  var inc: Int64 = 1

  while (rad > 2.0 * PI) {
    rad = rad - 2.0 * PI
  }
  while (rad < -2.0 * PI) {
    rad = rad + 2.0 * PI
  }
  app = rad
  diff = rad
  diff = (diff * (-(rad * rad))) / ((2.0 * Float64(inc)) * (2.0 * Float64(inc) + 1.0))
  app = app + diff
  inc = inc + 1
  while (fabs(diff) >= 0.00001) {
    diff = (diff * (-(rad * rad))) / ((2.0 * Float64(inc)) * (2.0 * Float64(inc) + 1.0))
    app = app + diff
    inc = inc + 1
  }

  return app
}

func cos(rad: Float64): Float64 {
  return sin(PI / 2.0 - rad)
}

func fft1(n: Int64, flag: Int64): Int64 {
  var i: Int64 = 0
  var j: Int64 = 0
  var k: Int64 = 0
  var it: Int64 = 0
  var xp: Int64 = 0
  var xp2: Int64 = 0
  var j1: Int64 = 0
  var j2: Int64 = 0
  var iter: Int64 = 0
  var sign: Float64 = 0.0
  var w: Float64 = 0.0
  var wr: Float64 = 0.0
  var wi: Float64 = 0.0
  var dr1: Float64 = 0.0
  var dr2: Float64 = 0.0
  var di1: Float64 = 0.0
  var di2: Float64 = 0.0
  var tr: Float64 = 0.0
  var ti: Float64 = 0.0
  var arg: Float64 = 0.0

  if (n < 2) {
    return 999
  }
  iter = Int64(log(Float64(n)) / log(2.0))
  j = 1
  i = 0
  while (i < iter) {
    j = j * 2
    i = i + 1
  }
  if (fabs(Float64(n - j)) > 1.0e-6) {
    return 1
  }

  /* Main FFT Loops */
  if (flag == 1) {
    sign = 1.0
  } else {
    sign = -1.0
  }
  xp2 = n
  it = 0
  while (it < iter) {
    xp = xp2
    xp2 = xp2 / 2
    w = PI / Float64(xp2)
    k = 0
    while (k < xp2) {
      arg = Float64(k) * w
      wr = cos(arg)
      wi = sign * sin(arg)
      i = k - xp
      j = xp
      while (j <= n) {
        j1 = j + i
        j2 = j1 + xp2
        dr1 = ar[j1]
        dr2 = ar[j2]
        di1 = ai[j1]
        di2 = ai[j2]
        tr = dr1 - dr2
        ti = di1 - di2
        ar[j1] = dr1 + dr2
        ai[j1] = di1 + di2
        ar[j2] = tr * wr - ti * wi
        ai[j2] = ti * wr + tr * wi
        j = j + xp
      }
      k = k + 1
    }
    it = it + 1
  }

  /* Digit Reverse Counter */
  j1 = n / 2
  j2 = n - 1
  j = 1
  i = 1
  while (i <= j2) {
    if (i < j) {
      tr = ar[j - 1]
      ti = ai[j - 1]
      ar[j - 1] = ar[i - 1]
      ai[j - 1] = ai[i - 1]
      ar[i - 1] = tr
      ai[i - 1] = ti
    }
    k = j1
    while (k < j) {
      j = j - k
      k = k / 2
    }
    j = j + k
    i = i + 1
  }
  if (flag == 0) {
    return 0
  }
  w = Float64(n)
  i = 0
  while (i < n) {
    ar[i] = ar[i] / w
    ai[i] = ai[i] / w
    i = i + 1
  }
  return 0
}

main(): Int64 {
  var i: Int64 = 0
  var n: Int64 = 8
  var flag: Int64 = 0
  var chkerr: Int64 = 0

  /* ar */
  i = 0
  while (i < n) {
    ar[i] = cos(2.0 * M_PI * Float64(i) / Float64(n))
    i = i + 1
  }

  /* forward fft */
  flag = 0
  chkerr = fft1(n, flag)

  /* inverse fft */
  flag = 1
  chkerr = fft1(n, flag)

  return 0
}
