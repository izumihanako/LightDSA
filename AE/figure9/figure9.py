from matplotlib.ticker import MultipleLocator, FixedLocator, LogLocator , NullFormatter
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

clusters = ['memmove' , 'memfill' , 'compare' , 'compval' ]
groups = [ "128K" , "256K" , "512K" , "1M" , "2M" , "4M" , "8M" ]
data = np.loadtxt('data.txt')

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

colors = ['#8c9e8e', '#d4b483', '#a6b0c5', '#d6a2ad'] 
hatch = [ '' , '///', '---', 'xxx' ]
for i in range(n_clusters):
    ax.bar(x_positions[i], data[:, i], 
          width=bar_width,
          color=colors[i],
            hatch=hatch[i],
          linewidth=0.3,
          label=clusters[i])

ax.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax.set_xticklabels(groups , fontsize=10)  
ax.xaxis.grid(False)
ax.spines['bottom'].set_color('black')
ax.set_xlabel('First descriptor transfer size (B)') 

ax.set_ylim( 4 , 1600 )
base = 2 
ax.set_yscale( "log" , base=base )
major_ticks = [ base**i for i in range( 2 , 12 , 2 )] 
ax.yaxis.set_major_locator( FixedLocator(major_ticks) )
minor_ticks = [ base**i for i in range( 1 , 11 , 2 )] 
ax.yaxis.set_minor_locator( FixedLocator(minor_ticks) ) 
ax.yaxis.set_minor_formatter(NullFormatter()) 

ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7)  
ax.spines['left'].set_color('black') 
ax.set_ylabel('OoO completion (log scale)   ')
 
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False) 
ax.legend(loc='upper left', fontsize=11 , ncol=2 , handlelength = 1.5 )

plt.tight_layout() 
plt.savefig('figure9.png', bbox_inches='tight', transparent=True) 
plt.savefig('figure9.pdf', bbox_inches='tight' , transparent=True)
plt.show()

# Out-of-order (OoO) completion when varying the first descriptor’s size in the first batch.
# All other sizes are 1 KB. The x-axis shows the first descriptor’s size; the y-axis (log scale) shows how many batches completed before the first.