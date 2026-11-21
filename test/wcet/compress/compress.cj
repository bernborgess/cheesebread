/* MDH WCET BENCHMARK SUITE. File version $Id: compress.c 7800 2018-01-19 12:43:12Z lkg02 $ */

let BUFFERSIZE: Int64 = 50
let IN_COUNT: Int64 = BUFFERSIZE

let HSIZE: Int64 = 257
let BITS: Int64 = 16
let INIT_BITS: Int64 = 9

let SACREDMEM: Int64 = 0
let USERMEM: Int64 = 450000
let PBITS: Int64 = 16

/* Second BITS define (same) */
let BIT_MASK: Int64 = 0x1f
let BLOCK_MASK: Int64 = 0x80

let CHECK_GAP: Int64 = 10000
let FIRST: Int64 = 257
let CLEAR: Int64 = 256

/* Macros */
/* Global variables */
var n_bits: Int64 = 0
var maxbits: Int64 = BITS
var maxcode: Int64 = 0
var maxmaxcode: Int64 = 1 << BITS

var hsize: Int64 = HSIZE

var free_ent: Int64 = 0
var exit_stat: Int64 = 0

var nomagic: Int64 = 1
var zcat_flg: Int64 = 0
var quiet: Int64 = 1

var block_compress: Int64 = BLOCK_MASK
var clear_flg: Int64 = 0
var ratio: Int64 = 0
var checkpoint: Int64 = CHECK_GAP

var force: Int64 = 0

var InCnt: Int64 = 0
var apsim_InCnt: Int64 = 0
var InBuff: Array<Int64> = Array<Int64>(0, repeat: 0)
var OutBuff: Array<Int64> = Array<Int64>(0, repeat: 0)
var inBuffOffset: Int64 = 0
var outBuffOffset: Int64 = 0

var orig_text_buffer: Array<Int64> = Array<Int64>(BUFFERSIZE, repeat: 0)
var comp_text_buffer: Array<Int64> = Array<Int64>(BUFFERSIZE + 5, repeat: 0)

var htab: Array<Int64> = Array<Int64>(HSIZE, repeat: 0)
var codetab: Array<Int64> = Array<Int64>(HSIZE, repeat: 0)
var buf: Array<Int64> = Array<Int64>(BITS, repeat: 0)

var offset: Int64 = 0

var in_count: Int64 = 1
var bytes_out: Int64 = 0
var out_count: Int64 = 0

var lmask: Array<Int64> = [0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00]
var rmask: Array<Int64> = [0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff]

func MAXCODE(n_bits: Int64): Int64 {
    return (1 << n_bits) - 1
}

/* assign helpers not functions; use direct arrays */

func initbuffer() {
    var seed: Int64 = 1
    var i: Int64 = 0
    var tabort: Int64 = 0
    for (i in 0..BUFFERSIZE) {
        tabort = i
        seed = ((seed * 133) + 81) % 8095
        orig_text_buffer[i] = seed % 256
    }
}

func cl_hash(hsize_param: Int64) {
    var htab_p: Int64 = hsize_param
    var i: Int64 = hsize_param - 16
    var m1: Int64 = -1
    do {
        htab[htab_p - 16] = m1
        htab[htab_p - 15] = m1
        htab[htab_p - 14] = m1
        htab[htab_p - 13] = m1
        htab[htab_p - 12] = m1
        htab[htab_p - 11] = m1
        htab[htab_p - 10] = m1
        htab[htab_p - 9] = m1
        htab[htab_p - 8] = m1
        htab[htab_p - 7] = m1
        htab[htab_p - 6] = m1
        htab[htab_p - 5] = m1
        htab[htab_p - 4] = m1
        htab[htab_p - 3] = m1
        htab[htab_p - 2] = m1
        htab[htab_p - 1] = m1
        htab_p = htab_p - 16
        i = i - 16
    } while (i >= 0)
    i = i + 16
    while (i > 0) {
        htab_p = htab_p - 1
        htab[htab_p] = m1
        i = i - 1
    }
}

func getbyte(): Int64 {
    if (InCnt > 0 && apsim_InCnt > 0) {
        apsim_InCnt = apsim_InCnt - 1
        InCnt = InCnt - 1
        var v: Int64 = InBuff[inBuffOffset]
        inBuffOffset = inBuffOffset + 1
        return v
    } else {
        return -1
    }
}

func putbyte(c: Int64) {
    OutBuff[outBuffOffset] = c & 0xff
    outBuffOffset = outBuffOffset + 1
}

func writebytes(buffer: Array<Int64>, n: Int64) {
    var i: Int64 = 0
    while (i < n && i < BITS) {
        OutBuff[outBuffOffset] = buffer[i] & 0xff
        outBuffOffset = outBuffOffset + 1
        i = i + 1
    }
}

func output(code: Int64) {
    var r_off: Int64 = offset
    var bits: Int64 = n_bits
    var bp: Int64 = 0

    if (code >= 0) {
        bp = r_off >> 3
        r_off = r_off & 7

        buf[bp] = ((buf[bp] & rmask[r_off]) | (code << r_off)) & lmask[r_off]
        /* note: original didn't mask code<<r_off before lmask? keep as is but mask final */
        bp = bp + 1
        bits = bits - (8 - r_off)
        var ccode: Int64 = code >> (8 - r_off)

        if (bits >= 8) {
            buf[bp] = ccode & 0xff
            bp = bp + 1
            ccode = ccode >> 8
            bits = bits - 8
        }

        if (bits != 0) {
            buf[bp] = ccode & 0xff
        }

        offset = offset + n_bits
        if (offset == (n_bits << 3)) {
            bp = 0
            bits = n_bits
            bytes_out = bytes_out + bits
            var k: Int64 = 0
            while (k < bits) {
                putbyte(buf[k])
                k = k + 1
            }
            offset = 0
        }

        if (free_ent > maxcode || clear_flg > 0) {
            if (offset > 0) {
                writebytes(buf, n_bits)
                bytes_out = bytes_out + n_bits
            }
            offset = 0
            if (clear_flg != 0) {
                n_bits = INIT_BITS
                maxcode = MAXCODE(n_bits)
                clear_flg = 0
            } else {
                n_bits = n_bits + 1
                if (n_bits == maxbits) {
                    maxcode = maxmaxcode
                } else {
                    maxcode = MAXCODE(n_bits)
                }
            }
        }
    } else {
        if (offset > 0) {
            writebytes(buf, (offset + 7) / 8)
        }
        bytes_out = bytes_out + (offset + 7) / 8
        offset = 0
    }
}

func cl_block() {
    var rat: Int64 = 0
    checkpoint = in_count + CHECK_GAP

    if (in_count > 0x007fffff) {
        rat = bytes_out >> 8
        if (rat == 0) {
            rat = 0x7fffffff
        } else {
            rat = in_count / rat
        }
    } else {
        rat = (in_count << 8) / bytes_out
    }
    if (rat > ratio) {
        ratio = rat
    } else {
        ratio = 0
        cl_hash(hsize)
        free_ent = FIRST
        clear_flg = 1
        output(CLEAR)
    }
}

func compress() {
    var fcode: Int64 = 0
    var i: Int64 = 0
    var c: Int64 = 0
    var ent: Int64 = 0
    var disp: Int64 = 0
    var hsize_reg: Int64 = 0
    var hshift: Int64 = 0

    offset = 0
    bytes_out = 3
    out_count = 0
    clear_flg = 0
    ratio = 0
    in_count = 1
    checkpoint = CHECK_GAP
    n_bits = INIT_BITS
    maxcode = MAXCODE(n_bits)
    free_ent = block_compress  // truthy nonzero => FIRST? C: free_ent = block_compress ? FIRST : 256
    if (block_compress != 0) {
        free_ent = FIRST
    } else {
        free_ent = 256
    }

    ent = getbyte()

    hshift = 0
    fcode = hsize
    while (fcode < 65536) {
        hshift = hshift + 1
        fcode = fcode * 2
    }
    hshift = 8 - hshift

    hsize_reg = hsize
    cl_hash(hsize_reg)

    while (InCnt > 0) {
        var apsim_bound111: Int64 = 0
        c = getbyte()
        in_count = in_count + 1
        fcode = (c << maxbits) + ent
        i = ((c << hshift) ^ ent)

        var found: Bool = false

        if (htab[i] == fcode) {
            ent = codetab[i]
            found = true
        } else if (htab[i] < 0) {
            // nomatch
        } else {
            disp = hsize_reg - i
            if (i == 0) {
                disp = 1
            }
            // probe loop
            while (true) {
                i = i - disp
                if (i < 0) {
                    i = i + hsize_reg
                }
                if (htab[i] == fcode) {
                    ent = codetab[i]
                    found = true
                    break
                }
                apsim_bound111 = apsim_bound111 + 1
                if (!(htab[i] > 0 && apsim_bound111 < in_count)) {
                    break
                }
            }
        }

        if (found) {
            continue
        }

        // nomatch
        out_count = out_count + 1
        ent = c
        if (free_ent < maxmaxcode) {
            codetab[i] = free_ent
            free_ent = free_ent + 1
            htab[i] = fcode
        } else if (in_count >= checkpoint && block_compress != 0) {
            cl_block()
        }
    }

    if (bytes_out > in_count) {
        exit_stat = 2
    }
}

main() {
    initbuffer()

    maxbits = BITS
    maxmaxcode = 1 << maxbits

    InCnt = IN_COUNT
    apsim_InCnt = IN_COUNT + 3
    InBuff = orig_text_buffer
    inBuffOffset = 0
    OutBuff = comp_text_buffer
    outBuffOffset = 0

    compress()
}
