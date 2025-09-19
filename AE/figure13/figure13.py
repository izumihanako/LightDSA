from matplotlib.ticker import MultipleLocator
from brokenaxes import brokenaxes
import matplotlib.pyplot as plt
import numpy as np
 
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({
    'font.family': 'Times New Roman',
    'font.size': 12,
    'axes.linewidth': 0.5,
    'grid.linewidth': 0.3,
    'figure.dpi': 300,
})
  
clusters = ['CPU core', 'naive DSA', 'LightDSA']
groups = ["7", "8", "9", "10", "11", "12", "13", "14"]
data = np.loadtxt('data.txt')
# data = np.array([
#         [0.3, 0.6, 1, 1.8, 3, 3.8, 4.3, 4.6],
#         [1.1, 2, 3.7, 6.8, 12, 19.1, 25, 26.7], 
#         [4.4, 7.8, 13.6, 22.9, 25.8, 26.6, 26.9, 27.2]]).T 

print( data[:,2] / data[:,1] )
# calculate speedup ratios compared to CPU core
speedup_naive = data[:, 1] / data[:, 0]  # naive DSA / CPU core
speedup_our = data[:, 2] / data[:, 0]    # Our lib / CPU core
 
fig, ax1 = plt.subplots(figsize=(5, 2.5)) 
 
ax2 = ax1.twinx()
 
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
 
colors = ['#8c9e8e', '#6A8DBA', '#a6b0c5'] 
colors.reverse()

ax1.set_ylim(0, 30) 
y_min = ax1.get_ylim()[0] 
broken_bar_height = 0.5 * y_min
broken_gap = 1

for i in range(n_clusters): 
    bars = ax1.bar(x_positions[i], data[:, i], 
          width=bar_width,
          color=colors[i], 
          linewidth=0.3,
          label=clusters[i], zorder=3) 
 
ax2.set_ylim(0, 15) 
ax2.plot(x_positions[1], speedup_naive, 'o--', color='#FDB813', markersize=4, linewidth=1, label='naive DSA speedup')
ax2.plot(x_positions[2], speedup_our, 's--', color='#9467BD', markersize=4, linewidth=1, label='LightDSA speedup')
 
ax1.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax1.set_xticklabels(groups, fontsize=11)  
ax1.xaxis.grid(False) 
ax1.set_xlabel('String length in $[2^n, 2^{n+1})$ (B)') 
ax1.spines['bottom'].set_color('black') 
  
ax1.yaxis.set_major_locator(MultipleLocator(4)) 
ax1.yaxis.set_minor_locator(MultipleLocator(2)) 
ax1.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7,zorder=0) 
ax1.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7,zorder=0) 
ax1.set_ylabel('Speed/device (GB/s)') 
ax1.spines['left'].set_color('black') 
ax2.spines['bottom'].set_color('black')  
 
ax2.set_ylabel('Speedup ratio (vs CPU core)')
ax2.yaxis.set_major_locator(MultipleLocator(2)) 
ax2.yaxis.set_minor_locator(MultipleLocator(1)) 
ax2.spines['right'].set_color('black') 
ax2.grid( visible=False) 
ax2.spines['bottom'].set_color('black') 
 
ax1.spines['top'].set_visible(False) 
ax2.spines['top'].set_visible(False) 
ax2.spines['left'].set_visible(False)  
 
lines1, labels1 = ax1.get_legend_handles_labels()
lines2, labels2 = ax2.get_legend_handles_labels()
ax2.legend(lines1 + lines2, labels1 + labels2, loc='upper left', ncol=1, borderpad=0, handlelength=1.5,
           fontsize=10, frameon=False)
  

plt.tight_layout()
 
plt.savefig('figure13.png', bbox_inches='tight', transparent=True)
plt.savefig('figure13.pdf', bbox_inches='tight') 
 
plt.show()

# Hash shuffle performance per device: average throughput per CPU core vs. individual DSA. 
# The x-axis shows string lengths in the range $[2^n, 2^{n+1})$ bytes. 
# Bars represent raw throughput (left y-axis), and lines indicate speedup over CPU core (right y-axis).