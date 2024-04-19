# ModelARX representations

## Binary

Format depends on platform's endianness. Will not work if moved between little-endian and big-endian platforms.

| size (bytes) | what | type |
| ------------ | ---- | ---- |
| 8 | n_coeff_a | `uint64_t` |
| 8 | n_coeff_b | `uint64_t` |
| 8 | dist_mean | `double` |
| 8 | dist_stddev | `double` |
| 8 | in_n | `uint64_t` |
| 8 | out_n | `uint64_t` |
| 8 | delay_n | `uint64_t` |
| 8 | init_seed | `uint64_t` |
| 8 | n_generated | `uint64_t` |
| n_coeff_a * 8 | coeff_a_data | `double[]` |
| n_coeff_b * 8 | coeff_b_data | `double[]` |
| in_n * 8 | input_q | `double[]` |
| out_n * 8 | output_q | `double[]` |
| delay_n * 8 | delay_q | `double[]` |

transport_delay is delay_n as `uint32_t`

## Text format (`operator<<` and `operator>>`)

The format is similar to inputs used in competitive programming and OI (Olimpiada Informatyczna). Should be easily movable between all possible systems.

The first line contains two space separated doubles - the distribution mean (should be 0) and standard deviation. The second line contains two unsigned integers. The first is seed (should be uint32_t, but is stored as uint64_t) and the second is the number of numbers generated so far. Then there are 10 lines in 5 pairs. The first line of each pair specifies the number of space separated doubles. Pairs are specified in the following order:

1. coeff_a
2. coeff_b
3. input_queue
4. output_queue
5. delay_queue

### Example

`ModelARX` defined by (values before converting to f64):

1. normal distribution with mean 0 and standard deviation 0.08
2. Mersenne Twister generator seeded with 3669609946 after 8 generations
3. `coeff_a` vector `{-0.4, 0.2}`
4. `coeff_b` vector `{0.6, 0.3}`
5. input queue `{-0.2, 2}`
6. output queue `{0.87857331718174558, 1.1626953251299641}`
7. delay queue `{0.36, -0.1}`

has the following text representation:

```
0.00000000000000000 0.08000000000000000
3669609946 8
2
-0.40000000000000002 0.20000000000000001
2
0.59999999999999998 0.29999999999999999
2
-0.20000000000000001 2
2
0.87857331718174558 1.1626953251299641
2
0.35999999999999999 -0.10000000000000001

```

### Notes

While there exist `operator<<` and `operator>>` functions for [random number engines](https://eel.is/c++draft/rand.req.eng) and [random number distributions](https://eel.is/c++draft/rand.req.dist), the format of the latter is not specified by standard and:

- libstdc++ and libc++ use different stream flags, although the results in my test were the same
- MSVC STL uses a completely [different format](https://github.com/microsoft/STL/blob/0515a05b394596de92d08cb0f352614479a2a883/stl/inc/random#L88-L101) for textual representation of `double` values
- the generator (which is represented in the same way, since the format is actually mentioned in the standard) can't be restored without distribution, because distibution's state depends on the generator
