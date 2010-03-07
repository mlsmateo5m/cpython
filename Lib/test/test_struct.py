import array
import unittest
import struct
import warnings
from test.test_support import run_unittest

import sys
ISBIGENDIAN = sys.byteorder == "big"
IS32BIT = sys.maxsize == 0x7fffffff

integer_codes = 'b', 'B', 'h', 'H', 'i', 'I', 'l', 'L', 'q', 'Q'

# Native 'q' packing isn't available on systems that don't have the C
# long long type.
try:
    struct.pack('q', 5)
except struct.error:
    HAVE_LONG_LONG = False
else:
    HAVE_LONG_LONG = True

def string_reverse(s):
    return "".join(reversed(s))

def bigendian_to_native(value):
    if ISBIGENDIAN:
        return value
    else:
        return string_reverse(value)

class StructTest(unittest.TestCase):

    def check_float_coerce(self, format, number):
        # SF bug 1530559. struct.pack raises TypeError where it used
        # to convert.
        with warnings.catch_warnings(record=True) as w:
            # ignore everything except the
            # DeprecationWarning we're looking for
            warnings.simplefilter("ignore")
            warnings.filterwarnings(
                "always",
                message=".*integer argument expected, got float",
                category=DeprecationWarning,
                module=__name__
                )
            got = struct.pack(format, number)
            nwarn = len(w)
        self.assertEqual(nwarn, 1,
                         "expected exactly one warning from "
                         "struct.pack({!r}, {!r});  "
                         "got {} warnings".format(
                format, number, nwarn))
        expected = struct.pack(format, int(number))
        self.assertEqual(got, expected)

    def test_isbigendian(self):
        self.assertEqual((struct.pack('=i', 1)[0] == chr(0)), ISBIGENDIAN)

    def test_consistence(self):
        self.assertRaises(struct.error, struct.calcsize, 'Z')

        sz = struct.calcsize('i')
        self.assertEqual(sz * 3, struct.calcsize('iii'))

        fmt = 'cbxxxxxxhhhhiillffd?'
        fmt3 = '3c3b18x12h6i6l6f3d3?'
        sz = struct.calcsize(fmt)
        sz3 = struct.calcsize(fmt3)
        self.assertEqual(sz * 3, sz3)

        self.assertRaises(struct.error, struct.pack, 'iii', 3)
        self.assertRaises(struct.error, struct.pack, 'i', 3, 3, 3)
        self.assertRaises(struct.error, struct.pack, 'i', 'foo')
        self.assertRaises(struct.error, struct.pack, 'P', 'foo')
        self.assertRaises(struct.error, struct.unpack, 'd', 'flap')
        s = struct.pack('ii', 1, 2)
        self.assertRaises(struct.error, struct.unpack, 'iii', s)
        self.assertRaises(struct.error, struct.unpack, 'i', s)

    def test_transitiveness(self):
        c = 'a'
        b = 1
        h = 255
        i = 65535
        l = 65536
        f = 3.1415
        d = 3.1415
        t = True

        for prefix in ('', '@', '<', '>', '=', '!'):
            for format in ('xcbhilfd?', 'xcBHILfd?'):
                format = prefix + format
                s = struct.pack(format, c, b, h, i, l, f, d, t)
                cp, bp, hp, ip, lp, fp, dp, tp = struct.unpack(format, s)
                self.assertEqual(cp, c)
                self.assertEqual(bp, b)
                self.assertEqual(hp, h)
                self.assertEqual(ip, i)
                self.assertEqual(lp, l)
                self.assertEqual(int(100 * fp), int(100 * f))
                self.assertEqual(int(100 * dp), int(100 * d))
                self.assertEqual(tp, t)

    def test_new_features(self):
        # Test some of the new features in detail
        # (format, argument, big-endian result, little-endian result, asymmetric)
        tests = [
            ('c', 'a', 'a', 'a', 0),
            ('xc', 'a', '\0a', '\0a', 0),
            ('cx', 'a', 'a\0', 'a\0', 0),
            ('s', 'a', 'a', 'a', 0),
            ('0s', 'helloworld', '', '', 1),
            ('1s', 'helloworld', 'h', 'h', 1),
            ('9s', 'helloworld', 'helloworl', 'helloworl', 1),
            ('10s', 'helloworld', 'helloworld', 'helloworld', 0),
            ('11s', 'helloworld', 'helloworld\0', 'helloworld\0', 1),
            ('20s', 'helloworld', 'helloworld'+10*'\0', 'helloworld'+10*'\0', 1),
            ('b', 7, '\7', '\7', 0),
            ('b', -7, '\371', '\371', 0),
            ('B', 7, '\7', '\7', 0),
            ('B', 249, '\371', '\371', 0),
            ('h', 700, '\002\274', '\274\002', 0),
            ('h', -700, '\375D', 'D\375', 0),
            ('H', 700, '\002\274', '\274\002', 0),
            ('H', 0x10000-700, '\375D', 'D\375', 0),
            ('i', 70000000, '\004,\035\200', '\200\035,\004', 0),
            ('i', -70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('I', 70000000L, '\004,\035\200', '\200\035,\004', 0),
            ('I', 0x100000000L-70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('l', 70000000, '\004,\035\200', '\200\035,\004', 0),
            ('l', -70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('L', 70000000L, '\004,\035\200', '\200\035,\004', 0),
            ('L', 0x100000000L-70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('f', 2.0, '@\000\000\000', '\000\000\000@', 0),
            ('d', 2.0, '@\000\000\000\000\000\000\000',
                       '\000\000\000\000\000\000\000@', 0),
            ('f', -2.0, '\300\000\000\000', '\000\000\000\300', 0),
            ('d', -2.0, '\300\000\000\000\000\000\000\000',
                       '\000\000\000\000\000\000\000\300', 0),
                ('?', 0, '\0', '\0', 0),
                ('?', 3, '\1', '\1', 1),
                ('?', True, '\1', '\1', 0),
                ('?', [], '\0', '\0', 1),
                ('?', (1,), '\1', '\1', 1),
        ]

        for fmt, arg, big, lil, asy in tests:
            for (xfmt, exp) in [('>'+fmt, big), ('!'+fmt, big), ('<'+fmt, lil),
                                ('='+fmt, ISBIGENDIAN and big or lil)]:
                res = struct.pack(xfmt, arg)
                self.assertEqual(res, exp)
                self.assertEqual(struct.calcsize(xfmt), len(res))
                rev = struct.unpack(xfmt, res)[0]
                if rev != arg:
                    self.assertTrue(asy)

    def test_calcsize(self):
        expected_size = {
            'b': 1, 'B': 1,
            'h': 2, 'H': 2,
            'i': 4, 'I': 4,
            'l': 4, 'L': 4,
            'q': 8, 'Q': 8,
            }

        # standard integer sizes
        for code in integer_codes:
            for byteorder in ('=', '<', '>', '!'):
                format = byteorder+code
                size = struct.calcsize(format)
                self.assertEqual(size, expected_size[code])

        # native integer sizes, except 'q' and 'Q'
        for format_pair in ('bB', 'hH', 'iI', 'lL'):
            for byteorder in ['', '@']:
                signed_size = struct.calcsize(byteorder + format_pair[0])
                unsigned_size = struct.calcsize(byteorder + format_pair[1])
                self.assertEqual(signed_size, unsigned_size)

        # bounds for native integer sizes
        self.assertTrue(struct.calcsize('b')==1)
        self.assertTrue(2 <= struct.calcsize('h'))
        self.assertTrue(4 <= struct.calcsize('l'))
        self.assertTrue(struct.calcsize('h') <= struct.calcsize('i'))
        self.assertTrue(struct.calcsize('i') <= struct.calcsize('l'))

        # tests for native 'q' and 'Q' when applicable
        if HAVE_LONG_LONG:
            self.assertEqual(struct.calcsize('q'), struct.calcsize('Q'))
            self.assertTrue(8 <= struct.calcsize('q'))
            self.assertTrue(struct.calcsize('l') <= struct.calcsize('q'))

    def test_integers(self):
        # Integer tests (bBhHiIlLqQ).
        import binascii

        class IntTester(unittest.TestCase):
            def __init__(self, format):
                super(IntTester, self).__init__(methodName='test_one')
                self.format = format
                self.code = format[-1]
                self.direction = format[:-1]
                if not self.direction in ('', '@', '=', '<', '>', '!'):
                    raise ValueError("unrecognized packing direction: %s" %
                                     self.direction)
                self.bytesize = struct.calcsize(format)
                self.bitsize = self.bytesize * 8
                if self.code in tuple('bhilq'):
                    self.signed = True
                    self.min_value = -(2L**(self.bitsize-1))
                    self.max_value = 2L**(self.bitsize-1) - 1
                elif self.code in tuple('BHILQ'):
                    self.signed = False
                    self.min_value = 0
                    self.max_value = 2L**self.bitsize - 1
                else:
                    raise ValueError("unrecognized format code: %s" %
                                     self.code)

            def test_one(self, x, pack=struct.pack,
                                  unpack=struct.unpack,
                                  unhexlify=binascii.unhexlify):

                format = self.format
                if self.min_value <= x <= self.max_value:
                    expected = long(x)
                    if self.signed and x < 0:
                        expected += 1L << self.bitsize
                    self.assertTrue(expected >= 0)
                    expected = '%x' % expected
                    if len(expected) & 1:
                        expected = "0" + expected
                    expected = unhexlify(expected)
                    expected = ("\x00" * (self.bytesize - len(expected)) +
                                expected)
                    if (self.direction == '<' or
                        self.direction in ('', '@', '=') and not ISBIGENDIAN):
                        expected = string_reverse(expected)
                    self.assertEqual(len(expected), self.bytesize)

                    # Pack work?
                    got = pack(format, x)
                    self.assertEqual(got, expected)

                    # Unpack work?
                    retrieved = unpack(format, got)[0]
                    self.assertEqual(x, retrieved)

                    # Adding any byte should cause a "too big" error.
                    self.assertRaises((struct.error, TypeError), unpack, format,
                                                                 '\x01' + got)
                else:
                    # x is out of range -- verify pack realizes that.
                    self.assertRaises(struct.error, pack, format, x)

            def run(self):
                from random import randrange

                # Create all interesting powers of 2.
                values = []
                for exp in range(self.bitsize + 3):
                    values.append(1L << exp)

                # Add some random values.
                for i in range(self.bitsize):
                    val = 0L
                    for j in range(self.bytesize):
                        val = (val << 8) | randrange(256)
                    values.append(val)

                # Values absorbed from other tests
                values.extend([300, 700000, sys.maxint*4])

                # Try all those, and their negations, and +-1 from
                # them.  Note that this tests all power-of-2
                # boundaries in range, and a few out of range, plus
                # +-(2**n +- 1).
                for base in values:
                    for val in -base, base:
                        for incr in -1, 0, 1:
                            x = val + incr
                            self.test_one(int(x))
                            self.test_one(long(x))

                # Some error cases.
                class NotAnIntNS(object):
                    def __int__(self):
                        return 42

                    def __long__(self):
                        return 1729L

                class NotAnIntOS:
                    def __int__(self):
                        return 85

                    def __long__(self):
                        return -163L

                for badobject in ("a string", 3+42j, randrange):
                    self.assertRaises((TypeError, struct.error),
                                      struct.pack, self.format,
                                      badobject)

                # an attempt to convert a non-integer (with an
                # implicit conversion via __int__) should succeed,
                # with a DeprecationWarning
                for nonint in NotAnIntNS(), NotAnIntOS():
                    with warnings.catch_warnings(record=True) as w:
                        # ignore everything except the
                        # DeprecationWarning we're looking for
                        warnings.simplefilter("ignore")
                        warnings.filterwarnings(
                            "always",
                            message=(".*integer argument expected, "
                                     "got non-integer.*"),
                            category=DeprecationWarning,
                            module=__name__
                            )
                        got = struct.pack(self.format, nonint)
                        nwarn = len(w)
                    self.assertEqual(nwarn, 1,
                                     "expected exactly one warning from "
                                     "struct.pack({!r}, {!r});  "
                                     "got {} warnings".format(
                            self.format, nonint, nwarn))
                    expected = struct.pack(self.format, int(nonint))
                    self.assertEqual(got, expected)

        byteorders = '', '@', '=', '<', '>', '!'
        for code in integer_codes:
            for byteorder in byteorders:
                if (byteorder in ('', '@') and code in ('q', 'Q') and
                    not HAVE_LONG_LONG):
                    continue
                format = byteorder+code
                t = IntTester(format)
                t.run()

    def test_p_code(self):
        # Test p ("Pascal string") code.
        for code, input, expected, expectedback in [
                ('p','abc', '\x00', ''),
                ('1p', 'abc', '\x00', ''),
                ('2p', 'abc', '\x01a', 'a'),
                ('3p', 'abc', '\x02ab', 'ab'),
                ('4p', 'abc', '\x03abc', 'abc'),
                ('5p', 'abc', '\x03abc\x00', 'abc'),
                ('6p', 'abc', '\x03abc\x00\x00', 'abc'),
                ('1000p', 'x'*1000, '\xff' + 'x'*999, 'x'*255)]:
            got = struct.pack(code, input)
            self.assertEqual(got, expected)
            (got,) = struct.unpack(code, got)
            self.assertEqual(got, expectedback)

    def test_705836(self):
        # SF bug 705836.  "<f" and ">f" had a severe rounding bug, where a carry
        # from the low-order discarded bits could propagate into the exponent
        # field, causing the result to be wrong by a factor of 2.
        import math

        for base in range(1, 33):
            # smaller <- largest representable float less than base.
            delta = 0.5
            while base - delta / 2.0 != base:
                delta /= 2.0
            smaller = base - delta
            # Packing this rounds away a solid string of trailing 1 bits.
            packed = struct.pack("<f", smaller)
            unpacked = struct.unpack("<f", packed)[0]
            # This failed at base = 2, 4, and 32, with unpacked = 1, 2, and
            # 16, respectively.
            self.assertEqual(base, unpacked)
            bigpacked = struct.pack(">f", smaller)
            self.assertEqual(bigpacked, string_reverse(packed))
            unpacked = struct.unpack(">f", bigpacked)[0]
            self.assertEqual(base, unpacked)

        # Largest finite IEEE single.
        big = (1 << 24) - 1
        big = math.ldexp(big, 127 - 23)
        packed = struct.pack(">f", big)
        unpacked = struct.unpack(">f", packed)[0]
        self.assertEqual(big, unpacked)

        # The same, but tack on a 1 bit so it rounds up to infinity.
        big = (1 << 25) - 1
        big = math.ldexp(big, 127 - 24)
        self.assertRaises(OverflowError, struct.pack, ">f", big)

    def test_1530559(self):
        # SF bug 1530559. struct.pack raises TypeError where it used to convert.
        for endian in ('', '>', '<'):
            for fmt in integer_codes:
                self.check_float_coerce(endian + fmt, 1.0)
                self.check_float_coerce(endian + fmt, 1.5)

    def test_unpack_from(self):
        test_string = 'abcd01234'
        fmt = '4s'
        s = struct.Struct(fmt)
        for cls in (str, buffer):
            data = cls(test_string)
            self.assertEqual(s.unpack_from(data), ('abcd',))
            self.assertEqual(s.unpack_from(data, 2), ('cd01',))
            self.assertEqual(s.unpack_from(data, 4), ('0123',))
            for i in xrange(6):
                self.assertEqual(s.unpack_from(data, i), (data[i:i+4],))
            for i in xrange(6, len(test_string) + 1):
                self.assertRaises(struct.error, s.unpack_from, data, i)
        for cls in (str, buffer):
            data = cls(test_string)
            self.assertEqual(struct.unpack_from(fmt, data), ('abcd',))
            self.assertEqual(struct.unpack_from(fmt, data, 2), ('cd01',))
            self.assertEqual(struct.unpack_from(fmt, data, 4), ('0123',))
            for i in xrange(6):
                self.assertEqual(struct.unpack_from(fmt, data, i), (data[i:i+4],))
            for i in xrange(6, len(test_string) + 1):
                self.assertRaises(struct.error, struct.unpack_from, fmt, data, i)

    def test_pack_into(self):
        test_string = 'Reykjavik rocks, eow!'
        writable_buf = array.array('c', ' '*100)
        fmt = '21s'
        s = struct.Struct(fmt)

        # Test without offset
        s.pack_into(writable_buf, 0, test_string)
        from_buf = writable_buf.tostring()[:len(test_string)]
        self.assertEqual(from_buf, test_string)

        # Test with offset.
        s.pack_into(writable_buf, 10, test_string)
        from_buf = writable_buf.tostring()[:len(test_string)+10]
        self.assertEqual(from_buf, test_string[:10] + test_string)

        # Go beyond boundaries.
        small_buf = array.array('c', ' '*10)
        self.assertRaises(struct.error, s.pack_into, small_buf, 0, test_string)
        self.assertRaises(struct.error, s.pack_into, small_buf, 2, test_string)

        # Test bogus offset (issue 3694)
        sb = small_buf
        self.assertRaises(TypeError, struct.pack_into, b'1', sb, None)

    def test_pack_into_fn(self):
        test_string = 'Reykjavik rocks, eow!'
        writable_buf = array.array('c', ' '*100)
        fmt = '21s'
        pack_into = lambda *args: struct.pack_into(fmt, *args)

        # Test without offset.
        pack_into(writable_buf, 0, test_string)
        from_buf = writable_buf.tostring()[:len(test_string)]
        self.assertEqual(from_buf, test_string)

        # Test with offset.
        pack_into(writable_buf, 10, test_string)
        from_buf = writable_buf.tostring()[:len(test_string)+10]
        self.assertEqual(from_buf, test_string[:10] + test_string)

        # Go beyond boundaries.
        small_buf = array.array('c', ' '*10)
        self.assertRaises(struct.error, pack_into, small_buf, 0, test_string)
        self.assertRaises(struct.error, pack_into, small_buf, 2, test_string)

    def test_unpack_with_buffer(self):
        # SF bug 1563759: struct.unpack doens't support buffer protocol objects
        data1 = array.array('B', '\x12\x34\x56\x78')
        data2 = buffer('......\x12\x34\x56\x78......', 6, 4)
        for data in [data1, data2]:
            value, = struct.unpack('>I', data)
            self.assertEqual(value, 0x12345678)

    def test_bool(self):
        for prefix in tuple("<>!=")+('',):
            false = (), [], [], '', 0
            true = [1], 'test', 5, -1, 0xffffffffL+1, 0xffffffff/2

            falseFormat = prefix + '?' * len(false)
            packedFalse = struct.pack(falseFormat, *false)
            unpackedFalse = struct.unpack(falseFormat, packedFalse)

            trueFormat = prefix + '?' * len(true)
            packedTrue = struct.pack(trueFormat, *true)
            unpackedTrue = struct.unpack(trueFormat, packedTrue)

            self.assertEqual(len(true), len(unpackedTrue))
            self.assertEqual(len(false), len(unpackedFalse))

            for t in unpackedFalse:
                self.assertFalse(t)
            for t in unpackedTrue:
                self.assertTrue(t)

            packed = struct.pack(prefix+'?', 1)

            self.assertEqual(len(packed), struct.calcsize(prefix+'?'))

            if len(packed) != 1:
                self.assertFalse(prefix, msg='encoded bool is not one byte: %r'
                                             %packed)

            for c in '\x01\x7f\xff\x0f\xf0':
                self.assertTrue(struct.unpack('>?', c)[0])

    if IS32BIT:
        def test_crasher(self):
            self.assertRaises(MemoryError, struct.pack, "357913941c", "a")



def test_main():
    run_unittest(StructTest)

if __name__ == '__main__':
    test_main()
