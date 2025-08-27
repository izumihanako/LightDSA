from matplotlib.ticker import MultipleLocator
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
 
tap_only_speed = 3.150 
clusters = ['memmove' , 'memfill' , 'compare' , 'compval' ]
groups = [ "PRS\n(a)" ,  "no\n(b)" , "all\n(c)" , "demand\n(d)" , "PF\n(e)" , 
           "demand\n(f)" , "PF\n(g)" ]
data = np.loadtxt('data.txt').T  # Load data from file
 
fig, ax = plt.subplots(figsize=(5, 2)) 
 
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
 
colors = ['#8c9e8e', '#d4b483', '#a6b0c5', '#d6a2ad'] 
hatch = [ '' , '///', '---', 'xxx' ]  
for i in range(n_clusters):
    ax.bar(x_positions[i], data[:, i], 
          width=bar_width,
          color=colors[i],
          hatch=hatch[i], 
          linewidth=0.3,
          label=clusters[i])

gray_start = cluster_centers[5] - gap_between_clusters/2 - gap_between_bars * 2 
current_xlim = ax.get_xlim() 
x_max = current_xlim[1] 
 
ax.axvspan(gray_start, x_max, 
           facecolor='#c0c0c0', alpha=0.3, zorder=0)
 
divider_position = cluster_centers[5] - gap_between_clusters / 2 - gap_between_bars * 2
ax.axvline(x=divider_position, color='gray', 
           linestyle='--', linewidth=0.8, alpha=0.7)
 
ax.set_xlim(current_xlim)

 
ax.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax.set_xticklabels(groups , fontsize=10)  
ax.xaxis.grid(False) 
ax.set_xlabel('Approaches to deal with page fault') 
ax.spines['bottom'].set_color('black') 
  
ax.set_yticks(np.arange(0, 11.1, 1)) 
ax.yaxis.set_major_locator(MultipleLocator(3))  
ax.yaxis.set_minor_locator(MultipleLocator(1))  
ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7) 
ax.set_ylabel('Throughput (GB/s)') 
ax.spines['left'].set_color('black') 

 
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False) 
ax.legend(loc='upper left', bbox_to_anchor = (0,1.1) ,  fontsize=11 , ncol=2 , handlelength = 1.5 ) 

plt.tight_layout()
 
plt.savefig('figure6.png', bbox_inches='tight', transparent=True)
plt.savefig('figure6.pdf', bbox_inches='tight')  
 
plt.show()

# DSA throughput under different page fault handling strategies: 
# (a) no pre-page-fault (PPF) with PRS enabled, (b) no PPF, (c) PPF all memory, (d) PPF on demand, 
# (e) page fault handling only (no DSA execution), (f) strategy (d) with 2 MB pages, and (g) strategy (e) with 2 MB pages.
 