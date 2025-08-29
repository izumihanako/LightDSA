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
 
tap_only_speed = 3.150  # 仅tap的速度
clusters = [ 'Naive DSA' , '+ LightDSA' , '+ Insighted Design' ]
groups = [ "1%" , "5%" , "25%" , "50%" , "75%" , "95%" , "99%" , "100%" ]
data = np.loadtxt('data.txt')
# data = np.array([ 
#         [ 0.411 , 0.562 , 1.009 , 1.446 , 1.498 , 1.253 , 1.037 , 0.913 ],
#         [ 0.396 , 0.654 , 1.602 , 2.388 , 3.359 , 2.83 , 2.702 , 2.515 ],
#         [ 0.395 , 0.543 , 0.958 , 1.401 , 1.439 , 1.174 , 0.945 , 0.824 ],
#         [ 0.396 , 0.538 , 0.932 , 1.343 , 1.331 , 1.079 , 0.845 , 0.716 ] ] ).T

# data includes time for CPU, naiveDSA, std_vector, nve_dsa_vector, my_dsa_vector
speed = 1 / data # calculate speed 
print( speed )
speed_up = [ ( speed[:, i] / speed[:, 0] - 1 ) for i in range(1, 4) ] # calculate speedup over CPU
data = np.array(speed_up).T * 100 # convert to percentage
print( data )
 
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
 
colors = ['#8c9e8e', '#6A8DBA'  , '#a6b0c5'] 
colors.reverse() 

ax.set_ylim( -5 , 28 ) 
y_min = ax.get_ylim()[0] 
broken_bar_height = 0.5 * y_min
broken_gap = 1
for i in range(n_clusters): 
    bars = ax.bar(x_positions[i], data[:, i], 
          width=bar_width,
          color=colors[i], 
          linewidth=0.3,
          label=clusters[i])
    for j in range( n_groups ):
            bar = bars[j]
            height = bar.get_height()  
            if height < y_min:  
                bar.set_height( broken_bar_height ) 
                ax.bar( x_positions[i,j], y_min - broken_bar_height - broken_gap ,
                    bottom = broken_bar_height - broken_gap,
                    width=bar_width, 
                    color=colors[i],  
                    linewidth=0.3 )
                ax.plot([ x_positions[i,j] - bar_width/2, x_positions[i,j] + bar_width/2], 
                    [ broken_bar_height , broken_bar_height - broken_gap ], 
                    color='black', linewidth=0.5) 
                ax.text(bar.get_x() + bar.get_width() / 2 - 0.07 , y_min * 0.6 ,
                    f'{height:.0f}', ha='center', va='top', color='gray', fontsize=10 )
 
ax.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax.set_xticklabels(groups , fontsize=11)  
ax.xaxis.grid(False) 
ax.set_xlabel('Percentage of append') 
ax.spines['bottom'].set_color('black') 
  
ax.yaxis.set_major_locator(MultipleLocator(6)) 
ax.yaxis.set_minor_locator(MultipleLocator(3)) 
ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7) 
ax.set_ylabel('Speed change vs CPU (%)')  
ax.spines['left'].set_color('black') 
 
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False) 
legend = ax.legend( loc='upper left', fontsize=12 , ncols = 1 , handlelength = 1.5 , handletextpad=0.5 ) 
 
plt.tight_layout()
 
plt.savefig('figure11.png', bbox_inches='tight', transparent=True)
plt.savefig('figure11.pdf', bbox_inches='tight') 

# Speedup over CPU implementation for three DSA-accelerated vector under varying append ratios. 
# Naive DSA uses DSA directly, LightDSA replaces direct usage with LightDSA library, and Insighted Design incorporates additional coding-level optimizations.