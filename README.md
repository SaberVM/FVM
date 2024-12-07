# FVM

The Functional Virtual Machine is a VM designed to be the target of functional languages. It is ridiculously easy to implement yourself and ridiculously easy to target with languages based on the lambda calculus. It is not trying to be the fastest possible, just very convenient. It uses a cloning-heavy approach to have no garbage collector and safe in-place mutation.

Currently, the core logic of a fetch-eval loop is working. `LAM 10 CAP 0 LAM 4 VAR 1 RET RET LIT 4 APP LIT 5 APP` is the bytecode of the expression `((\x.\y.x)(4))(5)`, so you can hopefully see how closely the source code maps onto the bytecode. No CPS conversion or hoisting is needed, and no closure conversion except only the calculation of free variables so that `CAP` instructions can build closures.

### Transition Rules

We're really using De Bruijn notation here, so it's more accurate to write the above expression as `((\.\.1)(#4))(#5)` where `#` marks that something is a literal instead of a variable. Intuitively, `VAR index` is a variable as its De Bruijn index, `CAP index` allows capturing a variable via its De Bruijn index for use in a closure, and `LAM n` means that the next `n` bytes are the body of a lambda abstraction, and should therefore be skipped over until that abstraction is applied to an argument.

The FVM is inspired by the CEK machine. It has the following transition rules:
```
|-; v,vals; env; captures| ~> v
|LIT n rest; vals; env; captures| ~> |rest; n,vals; env; captures|
|VAR n rest; vals; env; captures| ~> |rest; env[n],vals; env; captures|
|OWN n rest; vals; env; captures| ~> |rest; env[n],vals; env; captures|
|LAM n rest; vals; env; captures| ~> |rest[n:]; closure(rest,captures),vals; env; -|
|APP rest; v,closure(f_code,f_env),vals; env; captures| ~> |f_code; closure(rest,captures),vals; v,f_env; -|
|RET rest; v,closure(k_code,k_env),vals; env; captures| ~> |k_code; v,vals; k_env; -|
|CAP n rest; vals; env; captures| ~> |rest; vals; env; env[n],captures|
|ARR rest; n,vals; env; captures| ~> |rest; array(n),vals; env; captures|
|GET rest; i,arr,vals; env; captures| ~> |rest; arr[i],arr,vals; env; captures|
|SET rest; i,arr,v,vals; env; captures| ~> |rest; arr{i:=v},vals; env; captures|
|FST rest; r,l,vals; env; captures| ~> |rest; l,vals; env; captures|
|SND rest; r,l,vals; env; captures| ~> |rest; r,vals; env; captures|
|LET n rest; vals; env; captures| ~> |rest; vals; vals[n],env; n,captures|
```
`rest[n:]` means `rest` but skipping the first `n`-1 words (for example, `LAM 4 VAR 1 RET`, written `\.1` in De Bruijn notation, skips to directly after the `RET`). 

`OWN n` is the same as `VAR n` as you can see here, but it's more efficient and technically unsafe. Namely, it creates a new alias to the same physical memory, while `VAR n` gives a reference to a clone of the memory. In general `OWN n` is for if you know it's your last use of that variable, though I suspect there are some cases where even that can lead to a use-after-free, so use it sparingly until I can produce a comprehensive writeup on when exactly it's safe. When in doubt use `VAR n`, and know that the transition rules above are the formal spec, so we reserve the right to change `OWN n`'s behavior to be more aligned with that spec at any time.

`FST` and `SND` may seem a little backwards, because they name the stack values as `r` and then `l`, but this comes from the implementation of the stack as an array with a stack pointer that grows from the left to the right. This comes from the fact that lists in formal notation are written as growing from right to left, the opposite of typical array notation in software engineering. In each setting I use the convention of that setting, leading to this awkward situation here.

There are two strange things about this setting that actually cancel each other out, but deserve discussion. The first is the dual role of the stack as a call stack and a data stack. This means that hosted languages need to make sure there's only one value at the end of a scope, so that the value under it is the place to return to. The second is the presence of `FST` and `SND` instructions. They're actually very comfortable primitives for shrinking the stack until the top is just a return value and a return address. In this approach, instructions like `GET`, which put multiple things on the stack, can be conceptually thought of as putting a *pair* on the stack (conceptually a single value). The fetched array element is on top, so if you want to `SET` it into another array or something it's easy to do. But this allows us to think of the portion of the stack above the continuation as a single value we're constructing, and `FST` and `SND` elegantly extract the value of interest before returning. In my head I also pair (pun unintended) this reasoning with a so-far-informal connection to category theory, where the stack is an object in a cartesian category, and, because of the associativity of the cartesian product, we can equivalently think of pushing two values as pushing a pair (which necessarily supports a "first projection" morphism and "second projection" morphism) or pushing two values individually. It's like an isomorphism between a single-stack view and a stack-of-stacks view (stacks delimited by continuations to return to).

I use `LET n` instead of, say, `VAL n` (a hypothetical instruction that copies the `n`th value to the top of the stack) because the value stack is more of a scratch computation surface, and the the environment stack has better infrastructure for behaving like, well, an environment. That is, if a value is going to be referred to a few times, it should be assigned to a variable and referred to that way. Note that `LET n` doesn't clone the value, which should inform your decision on whether to refer to it with `OWN n` or `VAR n`.

### Development

The FVM implementation here is written as a simple C program, in `main.c` and `main.h`. The executable can be found at `/bin/fvm`, or built with `./build.sh`. `fvm` takes a binary (bytecode) file as a command-line argument. Examples of such files can be found in the `/tests/` directory. These were generated with a quite convenient Python eDSL in `test.py`. Running `python3 test.py` generates and runs the tests. This is a very simple hand-rolled unit test framework, designed for easily constructing FVM bytecode sequences to test. If you're working on the FVM codebase, you can define `DBG` as `1` to get a bunch of helpful debugging output during execution.

### TODO
- The arguments of `LAM` can be calculated by pairing the `LAM`s and `RET`s in a nested way. I'm not going to do this, but it does mean that the user-provided arguments to `LAM` can be statically checked, should an FVM implementation choose to do so. Equivalently we can think of this as checking that no instructions within a lambda abstraction body follow `RET`.
- Right now, if the bytecode is even slightly malformed it's undefined behavior. That's not really acceptable. In addition to the static analysis mentioned above, FVM implementations should check that the closure popped by `APP` is actually a closure, and halt execution if it isn't, and this behavior should be toggleable by the user so they can get a little more performance by running their own static analysis (Chez Scheme does this).
- We have integers, but we need arithmetic instructions, floats, and 1-byte numbers (which can pair with the aforementioned arrays to implement strings). We also ought to be using 63-bit and 31-bit integers instead of 32-bit integers, so we can have a single-bit tag to differentiate from closures and array pointers. (Which can distinguish from each other with a second bit.)
- We need a syscall instruction `SYS n` for side effects (`n` picks the syscall). It should be synchronous; see the next point.
- Parallelism: I propose a fork/join approach, conceptually a parallel tuple `(a || b)`. Parallel tuples are fork/join because the continuation of the tuple is the join point. I think because the FVM is both copy-heavy and not CPS, it makes a little more sense to take a BEAM process route than a JS callback route. I also like aligning the FVM with linear logic when the opportunity arises. But I don't want to require that implementations have all the apparatus of message-passing and whatnot. Parallel tuples present themselves as a great power-to-weight ratio: lazy implementations can simply evaluate `a` then `b` (or `b` then `a`) and put the results on the stack (first `a` then `b` no matter what). Then they can switch to an asynchronous/parallel approach later without breaking user code, since the user knows `a` and `b` aren't allowed to inter-depend at all. For asynchronous I/O the user can just run the synchronous I/O as one of the components of a parallel tuple, so other work happens at the same time in the other component. I imagine all of this just amounts to a `PAR n` instruction that, like `LAM n`, skips `n` bytes and continues execution, but then also executes the next `n` bytes asynchronously. We could consider a second instruction that runs two expressions and finishes as soon as either is done, discarding the other value.
- I ought to figure out some solution to exceptions, which is admittedly a pain point for the parallelism proposal above.
- I want a three more instructions, `REP`, `BRK`, and `CNT`, for implementing loops without growing the stack. `REP` does nothing on its own, `CNT` jumps back to a corresponding `REP` (respecting nesting) and `BRK` jumps to immediately after the soonest `CNT`. I prefer this over a tail call instruction, even though it's harder to implement and harder to target, because it still leaves room for a nice compilation from FVM bytecode to LLVM or C.
