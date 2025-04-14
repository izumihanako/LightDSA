# perf stat -e uncore_iio_0/event=0x41,umask=0x01/ \
#     -e unc_iio_iommu0.misses \
#     -e uncore_iio_0/event=0x41,umask=0x01/ \
#     -e unc_iio_iommu0.first_lookups \
#     numactl -C0 --membind=0 ./expr/pipeline/engine_ooo 0 16384 64 8192 0

perf stat -e uncore_iio_1/event=0x41,umask=0x01/ \
    -e  uncore_iio_0/event=0x41,umask=0x01/ \
    -e uncore_iio_5/event=0x41,umask=0x01/ \
    -e  uncore_iio_3/event=0x41,umask=0x01/ \
    -e uncore_iio_6/event=0x41,umask=0x01/ \
    -e  uncore_iio_7/event=0x41,umask=0x01/ \
    -e uncore_iio_8/event=0x41,umask=0x01/  \
    -e uncore_iio_9/event=0x41,umask=0x01/ \
    numactl -C0 --membind=0 ./expr/pipeline/engine_ooo 0 16384 64 8192 0