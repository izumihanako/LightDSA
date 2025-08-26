from matplotlib.ticker import MultipleLocator, FixedLocator, LogLocator , NullFormatter
import matplotlib.pyplot as plt
import numpy as np
import sys

# 设置紧凑型学术图表参数
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({
    'font.family': 'Times New Roman',
    'font.size': 12,
    'axes.linewidth': 0.5,
    'grid.linewidth': 0.3,
    'figure.dpi': 300,
})

# 数据准备
clusters = ['memmove' , 'memfill' , 'compare' , 'compval' ]
full_groups = [ "128" , "256" , "512" , "1K" , "2K" , "4K" , "8K" ]
dsa_full_data = np.loadtxt('dsa_data.txt')
cpu_full_data = np.loadtxt('cpu_data.txt') 
dsa_full_data = dsa_full_data / 1000  # turns to GB/s
cpu_full_data = cpu_full_data / 1000  # turns to GB/s

# n_display_start = 1
# n_display_end = 8
groups = full_groups # [n_display_start:n_display_end]
dsa_data = dsa_full_data # [n_display_start:n_display_end, :]
cpu_data = cpu_full_data # [n_display_start:n_display_end, :]

# create figure and axis
fig, ax = plt.subplots(figsize=(5, 2.3))

# bar settings
bar_width = 0.025  
gap_between_bars = 0.01 
gap_between_clusters = 0.1 

# cluster positions
n_groups = len(groups)
n_clusters = len(clusters)

# cluster center positions
cluster_centers = np.arange(n_groups) * (n_clusters * (bar_width + gap_between_bars) + gap_between_clusters)

# bar positions
x_positions = []
for i in range(n_clusters):
    x_positions.append(cluster_centers + i * (bar_width + gap_between_bars))
x_positions = np.array(x_positions)

# painting bars
colors_dsa = ['#8c9e8e', '#d4b483', '#a6b0c5', '#d6a2ad']
colors_cpu = ['#3a4a3e', '#7a5c32', '#4e5a75', '#8a4d5d'] 
hatch = [ '' , '///', '---', 'xxx' ]

for i in range(n_clusters):
    for j in range(n_groups):
        dsa_val = dsa_data[j, i]
        cpu_val = cpu_data[j, i] 
        # setting zorder according to values 
        if dsa_val > cpu_val:
            dsa_zorder = 2
            cpu_zorder = 3
        else:
            dsa_zorder = 3
            cpu_zorder = 2 
        # plot CPU bar
        ax.bar(x_positions[i][j], cpu_val, 
              width=bar_width,
              color=colors_cpu[i], 
              linewidth=0.5,
            #   hatch=hatch[i],
              zorder=cpu_zorder)
        
        # plot DSA bar
        ax.bar(x_positions[i][j], dsa_val, 
              width=bar_width,
              color=colors_dsa[i], 
              linewidth=0.5,
              hatch=hatch[i],
              zorder=dsa_zorder,
              label=clusters[i] if j == 0 else None)  # only add label for the first group

# set x-axis ticks and labels
ax.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax.set_xticklabels(groups , fontsize=10)  
ax.xaxis.grid(False)           # hide x-axis grid
ax.spines['bottom'].set_color('black') # set bottom spine color
ax.set_xlabel('Transfer size (B)') 

# set y-axis ticks
ax.set_yticks(np.arange(0, 33, 6))  # set y-axis major ticks
ax.yaxis.set_major_locator(MultipleLocator(6))  # major ticks
ax.yaxis.set_minor_locator(MultipleLocator(3))  # minor ticks
ax.yaxis.set_minor_formatter(NullFormatter())  # hide minor tick labels

ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7)  
ax.spines['left'].set_color('black') # set left spine color
ax.set_ylabel('Throughput (GB/s)')
 
# legend
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False)
from matplotlib.patches import Patch
legend_elements = [
    Patch(facecolor=colors_dsa[i], hatch=hatch[i], label=clusters[i]) for i in range(n_clusters)
] + [
    Patch( facecolor='#202020', hatch='', alpha=0.3, label='CPU')
]
ax.legend(handles=legend_elements, loc='upper left', fontsize=10, ncol=1)

plt.tight_layout()

# arg1 is figure3a / figure3b
plt.savefig( sys.argv[1] + '.png', bbox_inches='tight', transparent=True)
plt.savefig( sys.argv[1] + '.pdf', bbox_inches='tight' , transparent=True)
plt.show()