/* MDH WCET BENCHMARK SUITE. File version $Id: lms.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/* 2012/09/28, Jan Gustafsson <jan.gustafsson@mdh.se>
 * Changes:
 *  - This program redefines the C standard functions log, fabs, sqrt, and sin. Therefore, these function has been renamed with prefix lms_.
 *  - Warning: explicitly assigning a variable of type 'float' to itself
 *  x =x;
 *  removed.
 *  - Unused variables removed.
 */


/*************************************************************************/
/*                                                                       */
/*   SNU-RT Benchmark Suite for Worst Case Timing Analysis               */
/*   =====================================================               */
/*                              Collected and Modified by S.-S. Lim      */
/*                                           sslim@archi.snu.ac.kr       */
/*                                         Real-Time Research Group      */
/*                                        Seoul National University      */
/*                                                                       */
/*                                                                       */
/*        < Features > - restrictions for our experimental environment   */
/*                                                                       */
/*          1. Completely structured.                                    */
/*               - There are no unconditional jumps.                     */
/*               - There are no exit from loop bodies.                   */
/*                 (There are no 'break' or 'return' in loop bodies)     */
/*          2. No 'switch' statements.                                   */
/*          3. No 'do..while' statements.                                */
/*          4. Expressions are restricted.                               */
/*               - There are no multiple expressions joined by 'or',     */
/*                'and' operations.                                      */
/*          5. No library calls.                                         */
/*               - All the functions needed are implemented in the       */
/*                 source file.                                          */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  FILE: lms.c                                                          */
/*  SOURCE : C Algorithms for Real-Time DSP by P. M. Embree              */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     An LMS adaptive signal enhancement. The input signal is a sine    */
/*     wave with added white noise.                                      */
/*     The detailed description is in the program source code.           */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

let RAND_MAX: Float64 = 32768.0
let PI: Float64 = 3.14159265358979323846

let N: Int64 = 201
let L: Int64 = 20            /* filter order, (length L+1) */

/* set convergence parameter */
var mu: Float64 = 0.01

var next: Int64 = 1

func lms_rand(): Int64 {
    next = (next * 1103515245 + 12345) & 0xFFFFFFFF
    return (next / 65536) % 32768
}

func lms_log(r: Float64): Float64 {
    return 4.5
}

func lms_fabs(n: Float64): Float64 {
    var f: Float64 = 0.0

    if (n >= 0.0) {
        f = n
    } else {
        f = -n
    }
    return f
}

func lms_sqrt(val: Float64): Float64 {
    var x: Float64 = val / 10.0

    var dx: Float64 = 0.0

    var diff: Float64 = 0.0
    let min_tol: Float64 = 0.00001

    var i: Int64 = 0
    var flag: Int64 = 0

    flag = 0
    if (val == 0.0) {
        x = 0.0
    } else {
        i = 1
        while (i < 20) {
            if (flag == 0) {
                dx = (val - (x * x)) / (2.0 * x)
                x = x + dx
                diff = val - (x * x)
                if (lms_fabs(diff) <= min_tol) {
                    flag = 1
                }
            } else {
                /* JG */
            }
            i = i + 1
        }
    }
    return x
}

func lms_sin(radIn: Float64): Float64 {
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
    while (lms_fabs(diff) >= 0.00001) {
        diff = (diff * (-(rad * rad))) / ((2.0 * Float64(inc)) * (2.0 * Float64(inc) + 1.0))
        app = app + diff
        inc = inc + 1
    }

    return app
}

var gaussian_ready: Int64 = 0       /* flag to indicated stored value */
var gaussian_gstore: Float64 = 0.0  /* place to store other value */
let rconst1: Float64 = 2.0 / RAND_MAX
let rconst2: Float64 = RAND_MAX / 2.0

func gaussian(): Float64 {
    var v1: Float64 = 0.0
    var v2: Float64 = 0.0
    var r: Float64 = 0.0
    var fac: Float64 = 0.0
    var gaus: Float64 = 0.0

    /* make two numbers if none stored */
    if (gaussian_ready == 0) {
        v1 = Float64(lms_rand()) - rconst2
        v2 = Float64(lms_rand()) - rconst2
        v1 = v1 * rconst1
        v2 = v2 * rconst1
        r = v1 * v1 + v2 * v2
        while (r > 1.0) {
            v1 = Float64(lms_rand()) - rconst2
            v2 = Float64(lms_rand()) - rconst2
            v1 = v1 * rconst1
            v2 = v2 * rconst1
            r = v1 * v1 + v2 * v2
        }       /* make radius less than 1 */

        /* remap v1 and v2 to two Gaussian numbers */
        fac = lms_sqrt(-2.0 * lms_log(r) / r)
        gaussian_gstore = v1 * fac        /* store one */
        gaus = v2 * fac                   /* return one */
        gaussian_ready = 1               /* set ready flag */
    } else {
        gaussian_ready = 0               /* reset ready flag for next pair */
        gaus = gaussian_gstore           /* return the stored one */
    }

    return gaus
}

/*
      function lms(x,d,b,l,mu,alpha)

Implements NLMS Algorithm b(k+1)=b(k)+2*mu*e*x(k)/((l+1)*sig)

x      = input data
d      = desired signal
b[0:l] = Adaptive coefficients of lth order fir filter
l      = order of filter (> 1)
mu     = Convergence parameter (0.0 to 1.0)
alpha  = Forgetting factor   sig(k)=alpha*(x(k)**2)+(1-alpha)*sig(k-1)
         (>= 0.0 and < 1.0)

returns the filter output
*/

var lms_px: Array<Float64> = Array<Float64>(51, repeat: 0.0)      /* max L = 50 */
var lms_sigma: Float64 = 2.0                                      /* start at 2 and update internally */

func lms(x: Float64, d: Float64, b: Array<Float64>, l: Int64,
         mu: Float64, alpha: Float64): Float64 {
    var ll: Int64 = 0
    var e: Float64 = 0.0
    var mu_e: Float64 = 0.0
    var y: Float64 = 0.0

    lms_px[0] = x

    /* calculate filter output */
    y = b[0] * lms_px[0]
    ll = 1
    while (ll <= l) {
        y = y + b[ll] * lms_px[ll]
        ll = ll + 1
    }

    /* error signal */
    e = d - y

    /* update sigma */
    lms_sigma = alpha * (lms_px[0] * lms_px[0]) + (1.0 - alpha) * lms_sigma
    mu_e = mu * e / lms_sigma

    /* update coefficients */
    ll = 0
    while (ll <= l) {
        b[ll] = b[ll] + mu_e * lms_px[ll]
        ll = ll + 1
    }
    /* update history */
    ll = l
    while (ll >= 1) {
        lms_px[ll] = lms_px[ll - 1]
        ll = ll - 1
    }

    return y
}

main(): Int64 {
    var d: Array<Float64> = Array<Float64>(N, repeat: 0.0)
    var b: Array<Float64> = Array<Float64>(21, repeat: 0.0)
    var signal_amp: Float64 = 0.0
    var noise_amp: Float64 = 0.0
    var arg: Float64 = 0.0
    var x: Float64 = 0.0
    var k: Int64 = 0

    /* create signal plus noise */
    signal_amp = lms_sqrt(2.0)
    noise_amp = 0.2 * lms_sqrt(12.0)
    arg = 2.0 * PI / 20.0
    k = 0
    while (k < N) {
        d[k] = signal_amp * lms_sin(arg * Float64(k)) + noise_amp * gaussian()
        k = k + 1
    }

    /* scale based on L */
    mu = 2.0 * mu / Float64(L + 1)

    x = 0.0
    k = 0
    while (k < N) {
        lms(x, d[k], b, L, mu, 0.01)
        /* delay x one sample */
        x = d[k]
        k = k + 1
    }
    return 0
}
