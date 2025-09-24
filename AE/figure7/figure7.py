import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator
import numpy as np


import matplotlib
matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42
 
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({
    'font.family': 'Times New Roman',
    'font.size': 12,   
    'figure.dpi': 300,
    'axes.titlesize': 12,
    'axes.labelsize': 12
})
 
methods = [str(i) for i in range(1, 17)]
throughput1 = np.loadtxt('with_DRDBK_data.txt')  # MB/s
throughput1 /= 1000  # turns to GB/s
throughput2 = np.loadtxt('no_DRDBK_data.txt')  # MB/s
throughput2 /= 1000  # turns to GB/s
# throughput1 = [29.302, 29.316, 25.025, 21.363, 22.485, 23.391, 
#               24.059, 23.106, 23.000, 23.394, 23.799, 23.828,
#               23.635, 23.636, 23.790, 23.979]  # GB/s
# throughput2 = [ 29.381 , 29.397 , 29.391 , 29.400 , 29.385 , 29.393 ,
#                 29.388 , 29.393 , 29.391 , 29.390 , 29.391 , 29.395 ,
#                 29.397 , 29.388 , 29.389 , 29.393 ]  # GB/s

reversed_method = methods[::-1]
reversed_throughput1 = throughput1[::-1]
reversed_throughput2 = throughput2[::-1]
 
fig, ax = plt.subplots(figsize=(5, 2)) 
 
line1 = ax.plot(reversed_method, reversed_throughput1, color='#1f77b4', linewidth=1, 
                marker='x', markersize=6 , label='with DRDBK flag' , 
                 markerfacecolor='none', markeredgewidth=1 ) 
line2 = ax.plot(reversed_method, reversed_throughput2, color='#ff7f0e', linewidth=1,
                marker='o', markersize=6, label='without DRDBK flag' , 
                markerfacecolor='none', markeredgewidth=1 )

ax.set_ylabel('Throughput (GB/s)' )
ax.grid(axis='x', linestyle='-', linewidth=0.5 , alpha=0.7)

ax.spines['bottom'].set_color('black') 
ax.set_xlabel('Mix granularity')
  
ax.yaxis.set_major_locator(MultipleLocator(3)) 
ax.yaxis.set_minor_locator(MultipleLocator(1)) 
ax.grid(axis='y', which='major', linestyle='-', linewidth=0.5, alpha=0.7) 
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.4, alpha=0.7) 
ax.set_ylabel('Throughput (GB/s)') 
ax.spines['left'].set_color('black') 
 
ax.set_ylim( 21 , 30.5 )
ax.set_xticks(range(len(reversed_method))) 
 
ax.spines['top'].set_visible(False) 
ax.spines['right'].set_visible(False) 
ax.legend(loc='center left' , bbox_to_anchor=(0, 0.6),
          ncol=1 , borderpad=0.5, handlelength=1.5)
 
plt.tight_layout()
 
plt.savefig('figure7.png', bbox_inches='tight', transparent=True)
plt.savefig('figure7.pdf', bbox_inches='tight')

plt.show()

# caption: Mix granularity throughput comparison.
# Mix granularity is the number of consecutive large/small operations 
