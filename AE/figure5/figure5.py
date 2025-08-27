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
groups = [ "Major PF" , "Minor PF" , "ATC miss" , "No faults" ]
data = np.loadtxt('pf_type_data.txt').T

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
ax.text( cluster_centers[0] + 2 * bar_width , 0.1 , f'{"< 0.1"}', 
        ha='center', va='bottom', fontsize=11, color='#606060')
            
 
ax.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax.set_xticklabels(groups , fontsize=11)  
ax.xaxis.grid(False) 
ax.set_xlabel('Page fault type') 
ax.spines['bottom'].set_color('black') 
  
ax.set_yticks(np.arange(0, 33, 6)) 
ax.yaxis.set_major_locator(MultipleLocator(6)) 
ax.yaxis.set_minor_locator(MultipleLocator(3)) 
ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7) 
ax.set_ylabel('Throughput (GB/s)') 
ax.spines['left'].set_color('black') 
 
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False) 
ax.legend(loc='upper left', fontsize=11 , ncol=1 , handlelength = 1.5 ) 

plt.tight_layout() 
plt.savefig('figure5.png', bbox_inches='tight', transparent=True)
plt.savefig('figure5.pdf', bbox_inches='tight') 
 
plt.show()

# Impact of page faults and address translation cache (ATC) miss on DSA performance.
# Page faults are solved by default software handler.