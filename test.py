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
    def equals(self, expected):
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

def lam(bytes):
    return LAM + b(len(bytes) + 1) + bytes + RET

def lit(n):
    return LIT + b(n)

def var(n):
    return VAR + b(n)

def cap(n):
    return CAP + b(n)

def let(val, scope):
    return lam(scope)(val)

tests = [
    lam(cap(0) + lam(var(1)))(lit(3))(lit(4)).equals(3),
    let(lam(var(0)), var(0)(var(0)(lit(3)))).equals(3),
]

for i, test in enumerate(tests):
    filename = f"tests/test{i+1}.fvm"
    with open(filename, "wb") as f:
        f.write(test.code)
    out = subprocess.run(["bin/fvm", filename], stdout=subprocess.PIPE)
    if out.stdout == test.expected:
        print(f"Test {i+1} passed.")
    else:
        print(f"Test {i+1} failed.")
        print(f"Expected: {test.expected.decode('utf-8')}", end="")
        print(f"Got: {out.stdout.decode('utf-8')}")
        exit(1)