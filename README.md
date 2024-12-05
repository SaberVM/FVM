# FVM

The Functional Virtual Machine is a VM designed to be the target of functional languages. It is ridiculously easy to implement yourself and ridiculously easy to target with languages based on the lambda calculus. It is not trying to be the fastest possible, just very convenient. It uses a cloning-heavy approach to have no garbage collector and safe in-place mutation.