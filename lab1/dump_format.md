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
