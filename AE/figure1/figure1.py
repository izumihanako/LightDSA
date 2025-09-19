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

# prepare data
clusters = ['CPU' , 'Naive DSA' , 'LightDSA' ]
groups = [ "256" , "512" , "1K" , "2K" , "4K" , "8K" , "16K" , "32K" , "64K" , "128K" ]

# data = np.array(
#         [ [ 3687 , 4222 , 4719 , 5294 , 6166 , 7589 , 8296 , 8944 , 9318 , 9494 ],  
#         [ 442 , 717 , 1428 , 3424 , 5461 , 9894 , 15825 , 22883 , 26250 , 28143 ],
#         [ 1976 , 3941 , 7787 , 15254 , 28348 , 29110 , 29606 , 29796 , 29869 , 29903 ] ] ).T 
data = np.loadtxt('data.txt')  # 10 lines, 3 columns
data = data / 1000  # turns to GB/s  

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
colors = [ '#7a5c32' , '#b5a5cc'  , '#8c9e8e' ] 
hatch_pattern = ["///", "xxx" , "" ] 

for i in range(n_clusters):
    ax.bar(x_positions[i], data[:, i], 
          width=bar_width,
          color=colors[i],
          hatch=hatch_pattern[i],
          linewidth=0.3,
          label=clusters[i]) 

# Add ideal performance line
ideal_perf = 31
ax.axhline(y=ideal_perf , color="black" , linestyle='--', linewidth=1, alpha=0.7)
ax.text( -0.05 , ideal_perf - 3 , 'Ideal Performance of DSA', fontsize=12, va='center')

# set x-axis ticks and labels
ax.set_xticks(cluster_centers + (n_clusters-1)*(bar_width+gap_between_bars)/2)
ax.set_xticklabels(groups,fontsize=11)  
ax.xaxis.grid(False)           # hide x-axis grid
ax.spines['bottom'].set_color('black') # set bottom spine color
ax.set_xlabel('Transfer size (B)') 

# set y-axis ticks
ax.set_yticks(np.arange(0, 33, 6))  # set y-axis major ticks
ax.yaxis.set_major_locator(MultipleLocator(6))  # major ticks
ax.yaxis.set_minor_locator(MultipleLocator(3))  # minor ticks (optional)
ax.yaxis.set_minor_formatter(NullFormatter())  # hide labels

ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7)  
ax.spines['left'].set_color('black') # set left spine color
ax.set_ylabel('Throughput (GB/s)')
 
# legend settings
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False)
legend = ax.legend(loc='center left', fontsize=11, ncols=1, 
                  handlelength=1.5, handletextpad=0.5,
                  framealpha=0.9, edgecolor='black') 

plt.tight_layout()
plt.savefig('figure1.png', bbox_inches='tight', transparent=True)   
plt.savefig('figure1.pdf', bbox_inches='tight' , transparent=True)  
plt.show()