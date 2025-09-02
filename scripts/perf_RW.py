import subprocess
import sys

name_mapping = {
    "dsa0/event_category=0x1,event=0x1/": "Total input data  (32 bytes)", 
    "dsa0/event_category=0x1,event=0x2/": "Total data written(32 bytes)",
    "dsa0/event_category=0x2,event=0x1/": "succ trans no pf", 
    "dsa0/event_category=0x2,event=0x10/": "PRS Responses success",
    "unc_iio_iommu0.first_lookups": "IOMMU TLB unique lookup",
    "unc_iio_iommu0.ctxt_cache_lookups": "IOMMU TLB lookup misses",
    # "unc_iio_iommu0.misses": "IOMMU TLB misses", # same as above
    "unc_iio_iommu1.num_mem_accesses": "IOMMU memory accesses", 
}

def run_perf_stat(command):
    perf_cmd = [
        "perf", "stat" , 
        "-e", "dsa0/event_category=0x1,event=0x1/",
        "-e", "dsa0/event_category=0x1,event=0x2/", 
        "-e", "dsa0/event_category=0x2,event=0x1/",  
        "-e", "dsa0/event_category=0x2,event=0x10/", 
        "-e", "unc_iio_iommu0.first_lookups" ,
        "-e", "unc_iio_iommu0.ctxt_cache_lookups" ,
        # "-e", "unc_iio_iommu0.misses" ,
        "-e", "unc_iio_iommu1.num_mem_accesses" , 
    ] + command.split()
    try:
        result = subprocess.run(
            perf_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,  # perf stat输出到stderr
            text=True,
            check=True
        )
        print( result.stdout )  # 打印stdout
        return result.stderr
    except subprocess.CalledProcessError as e:
        print(f"Error running perf: {e}")
        return None

def parse_perf_stat(output):
    metrics = {}
    for line in output.split('\n'):
        if '#' in line:
            continue  # 跳过注释行
        parts = line.strip().split()
        if len(parts) >= 2:
            value = parts[0].replace(',', '')
            name = ' '.join(parts[1:2])
            percent="(100.00%)"
            if len(parts) > 2:
                percent = parts[2].replace(',', '')
            if value.isdigit() or value.replace('.', '').isdigit():
                metrics[name] = float(value) if '.' in value else int(value)
                metrics[name + " percent"] = percent
    return metrics

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 perf_PIPE.py <command>")
        sys.exit(1)

    command = sys.argv[1]
    output = run_perf_stat(command)

    if output:
        # print("Perf stat output:")
        # print(output)
        stats = parse_perf_stat( output )  
        print("Perf metrics:")
        for name, value in stats.items():
            if name in name_mapping:
                print( f"{name_mapping[name]:<30}: {value:<8} , {stats[name + ' percent']}" )