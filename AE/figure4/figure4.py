from matplotlib.ticker import MultipleLocator
import matplotlib.pyplot as plt
import numpy as np


import matplotlib
matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42
 
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({
    'font.family': 'Times New Roman',
    'font.size': 12,
    'axes.linewidth': 0.5,
    'grid.linewidth': 0.3,
    'figure.dpi': 300,
})
  
clusters = ['CPU' , 'Naive DSA' , 'LightDSA' ]
groups = [ "250" , "500" , "1K", "2K", "4K", "8K", "16K", "32K", "64K", "128K" ]
data = np.loadtxt('data.txt') 
data = data / 1000 
 
fig, ax = plt.subplots(figsize=(5, 2.3)) 
 
bar_width = 0.025 
gap_between_bars = 0.01  
gap_between_clusters = 0.1 
 
n_groups = len(groups)
n_clusters = len(clusters)
 
cluster_centers = np.arange(n_groups) * (n_clusters * (bar_width + gap_between_bars) + gap_between_clusters)
 
x_positions = []
for i in range(n_clusters):
    x_positions.append(cluster_centers + i * (bar_width + gap_between_bars))
x_positions = np.array(x_positions)
 
colors = [ '#7a5c32' , '#b5a5cc'  , '#8c9e8e' ] 
hatch_pattern = ["///", "xxx" , "" ] 
for i in range(n_clusters):
    ax.bar(x_positions[i], data[:, i], 
          width=bar_width,
          color=colors[i], 
          hatch=hatch_pattern[i], 
          linewidth=0.3,
          label=clusters[i])
 
ax.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax.set_xticklabels(groups , fontsize=11)  
ax.xaxis.grid(False)  
ax.set_xlabel('Number of memmove operations') 
ax.spines['bottom'].set_color('black') 
  
ax.set_yticks(np.arange(0, 18, 4)) 
ax.yaxis.set_major_locator(MultipleLocator(4)) 
ax.yaxis.set_minor_locator(MultipleLocator(2)) 
ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7) 
ax.set_ylabel('Throughput (GB/s)') 
ax.spines['left'].set_color('black') 

 
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False) 
handles, labels = ax.get_legend_handles_labels()
order = [2, 1, 0] 
ax.legend( [handles[idx] for idx in order], 
          [labels[idx] for idx in order] , loc='upper left', fontsize=11 , ncol=1 , handlelength = 1.5 ) 

plt.tight_layout()
 
plt.savefig('figure4.png', bbox_inches='tight', transparent=True)
plt.savefig('figure4.pdf', bbox_inches='tight') 
plt.show()