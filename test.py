import subprocess

class Test:
    def __init__(self, code):
        self.code = code
        self.expected = b''

class Bytes:
    def __init__(self, value):
        self.value = value
    def __add__(self, other):
        return Bytes(self.value + other.value)
    def __call__(self, *args):
        return self + args[0] + APP
    def __str__(self):
        return "Bytes(" + str(self.value) + ")"
    __repr__ = __str__
    def __len__(self):
        return len(self.value)
    def __getitem__(self, i):
        return self + lit(i) + GET
    def set(self, i, v):
        return v + self + lit(i) + SET
    def and_(self, other):
        return self + other + AND
    def or_(self, other):
        return self + NOT + other + NOT + AND + NOT
    def plus(self, other):
        return self + other + ADD
    def minus(self, other):
        return self + other + SUB
    def times(self, other):
        return self + other + MUL
    def greater_than(self, other):
        return self + other + GRT
    def __eq__(self, expected):
        test = Test(self.value)
        test.expected = str(expected).encode() + b'\n'
        return test


def b(i):
    return Bytes(i.to_bytes(1, 'big'))

LAM = b(0)
APP = b(1)
VAR = b(2)
RET = b(3)
LIT = b(4)
CAP = b(5)
OWN = b(6)
ARR = b(7)
GET = b(8)
SET = b(9)
FST = b(10)
SND = b(11)
LET = b(12)
LEN = b(13)
ADD = b(14)
SUB = b(15)
MUL = b(16)
DIV = b(17)
MOD = b(18)
EQL = b(19)
GRT = b(20)
AND = b(21)
NOT = b(23)
JIF = b(24)
REP = b(25)
BRK = b(26)
CNT = b(27)
PAR = b(28)
SYS = b(29)
CUT = b(30)

def lam(bytes):
    return LAM + b(len(bytes) + 1) + bytes + RET

def lit(n):
    return LIT + b(n)

def var(n):
    return VAR + b(n)

def cap(n):
    return CAP + b(n)

def own(n):
    return OWN + b(n)

def arr(n):
    return lit(n) + ARR

def fst(val):
    return val + FST

def snd(val):
    return val + SND

def let(val, scope):
    return val + LET + b(0) + scope

def len_(val):
    return val + LEN

def add(a, b):
    return a + b + ADD

def sub(a, b):
    return a + b + SUB

def mul(a, b):
    return a + b + MUL

def div(a, b):
    return a + b + DIV

def mod(a, b):
    return a + b + MOD

def eql(a, b):
    return a + b + EQL

def grt(a, b):
    return a + b + GRT

def not_(a):
    return a + NOT

def jif(cond, skipped):
    return cond + JIF + b(len(skipped)) + skipped

def cut(n):
    return CUT + b(n)

def while_(cond, body):
    scope = let(REP, jif(cond, own(0) + BRK + b(len(body) + 4)) + body + cut(1))
    return scope + CNT + b(len(scope) - 1)

def par(left, right):
    return PAR + b(len(left)) + b(len(right)) + left + right

def print_(val):
    return val + SYS + b(0)

tests = [
    print_(lam(cap(0) + lam(var(1)))(lit(3))(lit(4))) == 3,
    print_(lam(cap(0) + lam(own(1)))(lit(3))(lit(4))) == 3,
    print_(let(lam(var(0)), var(0)(var(0)(lit(3))))) == 3,
    print_(let(lam(own(0)), var(0)(var(0)(lit(3))))) == 3,
    print_(let(arr(2), snd(var(0).set(0, lit(4)).set(1, lit(3))[1]))) == 3,
    print_(lit(100) + lit(3) + fst(while_(own(0).greater_than(lit(0)), lit(100)+MUL+own(0).minus(lit(1)))) + while_(own(0).greater_than(lit(3)), own(0).minus(lit(1)))) == 3,
    print_(fst(par(lit(3), lit(4)))) == 3,
    print_(jif(not_(lit(0).and_(lit(0).or_(lit(1)))), print_(lit(2))) + lit(3)) == 3,
]

for i, test in enumerate(tests):
    filename = f"tests/test{i+1}.fvm"
    with open(filename, "wb") as f:
        f.write(test.code)
for i, test in enumerate(tests):
    filename = f"tests/test{i+1}.fvm"
    out = subprocess.run(["bin/fvm", filename], stdout=subprocess.PIPE)
    if out.stdout == test.expected:
        print(f"Test {i+1} passed.")
    else:
        print(f"Test {i+1} failed.")
        print(f"Expected: {test.expected.decode('utf-8')}", end="")
        print(f"Got: {out.stdout.decode('utf-8')}")
        exit(1)