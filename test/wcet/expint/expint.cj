/* MDH WCET BENCHMARK SUITE. File version $Id: expint.c 7800 2018-01-19 12:43:12Z lkg02 $ */

main() {
  expint(50, 1)
}

func foo(x: Int64): Int64 {
  return (x * x + (8 * x)) << (4 - x)
}

func expint(n: Int64, x: Int64): Int64 {
  var i: Int64 = 0
  var ii: Int64 = 0
  var nm1: Int64 = 0
  var a: Int64 = 0
  var b: Int64 = 0
  var c: Int64 = 0
  var d: Int64 = 0
  var del: Int64 = 0
  var fact: Int64 = 0
  var h: Int64 = 0
  var psi: Int64 = 0
  var ans: Int64 = 0

  nm1 = n - 1

  if (x > 1) {
    b = x + n
    c = 2000000
    d = 30000000
    h = d

    i = 1
    while (i <= 100) {
      a = -i * (nm1 + i)
      b = b + 2
      d = 10 * (a * d + b)
      c = b + a / c
      del = c * d
      h = h * del
      if (del < 10000) {
        ans = h * (-x)
        return ans
      }
      i = i + 1
    }
  } else {
    if (nm1 != 0) {
      ans = 2
    } else {
      ans = 1000
    }
    fact = 1
    i = 1
    while (i <= 100) {
      fact = fact * ((-x) / i)
      if (i != nm1) {
        del = (-fact) / (i - nm1)
      } else {
        psi = 0x00FF
        ii = 1
        while (ii <= nm1) {
          psi = psi + ii + nm1
          ii = ii + 1
        }
        del = psi + fact * foo(x)
      }
      ans = ans + del
      i = i + 1
    }
  }

  return ans
}