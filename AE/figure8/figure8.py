from matplotlib.ticker import MultipleLocator, FixedLocator, LogLocator , NullFormatter
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
 
clusters = ['16B, non-batch', '32B, non-batch', '64B, non-batch',
            '16B, batch'    , '32B, batch'    , '64B, batch']
groups = [ "64" , "128" , "256" , "512" , "1K" , "2K" ]
data = np.loadtxt('data.txt') 
data = data / 1000 
 
fig, ax = plt.subplots(figsize=(5, 2.5)) 

 
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
 
colors = ['#002b5c', '#1a5f9c', '#5d9ce2', 
          '#023020', '#355e3b', '#7cb083', 
         ] 

ax.set_ylim( 0 , 10.2 ) 
y_max = ax.get_ylim()[1] 
broken_bar_height = 0.85 * y_max
broken_gap = 0.2 
for i in range(n_clusters):
    bars = ax.bar(x_positions[i], data[:, i], 
          width=bar_width,
          color=colors[i], 
          linewidth=0.3,
          hatch = "///" if i < 3 else None , 
          label=clusters[i]) 
    for j in range( n_groups ):
        bar = bars[j]
        height = bar.get_height()  
        if height > y_max:  
            bar.set_height( broken_bar_height ) 
            ax.bar( x_positions[i,j], y_max - broken_bar_height - broken_gap ,
                bottom = broken_bar_height + broken_gap,
                width=bar_width,
                hatch = "///" if i < 3 else None ,  
                color=colors[i],  
                linewidth=0.3 )
            ax.plot([ x_positions[i,j] - bar_width/2, x_positions[i,j] + bar_width/2], 
                [ broken_bar_height , broken_bar_height + broken_gap ], 
                color='black', linewidth=0.5)  
            ax.text(bar.get_x() + bar.get_width() / 2, ax.get_ylim()[1] * 1.06 ,
                   f'{height:.1f}', ha='center', va='top', color='black', fontsize=12, fontweight='bold')
 
ax.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax.set_xticklabels(groups , fontsize=12)  
ax.xaxis.grid(False)  
ax.set_xlabel('Transfer size (B)')
ax.spines['bottom'].set_color('black') 
  
ax.yaxis.set_major_locator(MultipleLocator(2))  
ax.yaxis.set_minor_locator(MultipleLocator(1))  
ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7) 
ax.set_ylabel('Throughput (GB/s)') 
ax.spines['left'].set_color('black') 
 
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False) 
ax.legend(loc='upper left', fontsize=10 , ncol=2 , handlelength = 1.25 )  

plt.tight_layout()
plt.savefig('Chap3_combine_memmove.png', bbox_inches='tight', transparent=True)  
plt.savefig('Chap3_combine_memmove.pdf', bbox_inches='tight' , transparent=True) 
plt.show()


# Throughput comparison of \texttt{memmove} operations with different write address alignment granularities (16B, 32B, 64B) and submission modes. 
# Green bars represent descriptors submitted in batches, while blue bars represent descriptors submitted individually.