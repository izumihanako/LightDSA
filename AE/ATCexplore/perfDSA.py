import subprocess
import sys

name_mapping = {
    "dsa0/event_category=0x2,event=0x40/": "Translation requests",
    "dsa0/event_category=0x2,event=0x100/": "Translation hits", 
}

def run_perf_stat(command):
    perf_cmd = [
        "perf", "stat" , 
        "-e", "dsa0/event_category=0x2,event=0x40/",
        "-e", "dsa0/event_category=0x2,event=0x100/",  
    ] + command.split() 
    try:
        result = subprocess.run(
            perf_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,  # perf stat输出到stderr
            text=True,
            check=True
        )
        print( result.stdout )  # 打印stdout输出
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
                print( f"{name_mapping[name]:<22}: {value:<3} , {stats[name + ' percent']}" )