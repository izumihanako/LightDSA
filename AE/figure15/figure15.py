import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from matplotlib import colors as mcolors 
from matplotlib.ticker import FuncFormatter
from matplotlib.patches import Rectangle 

import matplotlib
matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({
    'font.family': 'Times New Roman',
    'font.size': 13,
    'axes.linewidth': 0.5,
    'grid.linewidth': 0.3,
    'figure.dpi': 300,
})

drdbk_data = np.loadtxt('with_DRDBK_data.txt', delimiter=',')

# 转换数据单位 (MB/s -> GB/s)
Z_drdbk = drdbk_data / 1000

# 创建单图
fig, ax = plt.subplots(figsize=(5, 3.5))

# 颜色范围设置
vmin = Z_drdbk.min()
vmax = Z_drdbk.max()

# 检查范围并打印出来以便调试
print(f"Data range: {vmin:.2f} - {vmax:.2f}")

custom_cmap = mcolors.LinearSegmentedColormap.from_list("red_to_green", [
    "mediumblue", "PapayaWhip", "Red"
])

# 绘制热力图 - 移除平滑插值
heatmap = ax.imshow(Z_drdbk, cmap=custom_cmap, origin='lower', 
                   aspect='auto', interpolation='nearest',  # 使用nearest表示无插值
                   extent=[0, Z_drdbk.shape[1], 0, Z_drdbk.shape[0]], vmin=vmin, vmax=vmax)
rect = Rectangle((0, 0), 1, 1, fill=True, 
                 facecolor='lightgray', alpha=0.7,  # 灰色半透明底色
                 hatch='///', edgecolor='black',    # 添加斜线图案
                 linewidth=0.5)
ax.add_patch(rect)
ax.set_title('With DRDBK Flag', fontweight='bold')

ax.set_title('With DRDBK Flag', fontweight='bold')
ax.set_ylabel('Small Operations Count')
ax.set_xlabel('Big Operations Count')

# 设置 x 和 y 轴为整数刻度
ax.xaxis.set_major_locator(ticker.MultipleLocator(2))
ax.xaxis.set_minor_locator(ticker.MultipleLocator(1)) 
ax.grid(axis='x', which='major', linestyle='-', linewidth=0.3, alpha=0.7, color='white') 
ax.grid(axis='x', which='minor', linestyle=':', linewidth=0.3, alpha=0.7, color='white') 
ax.yaxis.set_major_locator(ticker.MultipleLocator(2))
ax.yaxis.set_minor_locator(ticker.MultipleLocator(1))
ax.grid(axis='y', which='major', linestyle='-', linewidth=0.3, alpha=0.7, color='white')
ax.grid(axis='y', which='minor', linestyle=':', linewidth=0.3, alpha=0.7, color='white') 
# 应用自定义格式化器
# 定义格式化函数，用于添加 "S" 和 "B" 后缀
def format_y_axis(x, pos):
    return f"{int(x)}S" if x.is_integer() else f"{x}S"

def format_x_axis(x, pos):
    return f"{int(x)}B" if x.is_integer() else f"{x}B"
ax.xaxis.set_major_formatter(FuncFormatter(format_x_axis))
ax.yaxis.set_major_formatter(FuncFormatter(format_y_axis))

# 添加颜色条并设置刻度，强调22-25区间
cbar = fig.colorbar(heatmap, ax=ax, shrink=0.8, pad=0.02, label='Throughput (GB/s)')

# 为颜色条设置特定刻度，强调22-25区间
ticks = [round(vmin,1), 22, 23, 24, 25, 26, 28, round(vmax,1)]
cbar.set_ticks(ticks)

# 查找峰值和谷值
max_pos = np.unravel_index(Z_drdbk.argmax(), Z_drdbk.shape)
min_pos = np.unravel_index(Z_drdbk.argmin(), Z_drdbk.shape)

# 标注重要点
max_value_normalized = (Z_drdbk[max_pos] - vmin) / (vmax - vmin)  # 归一化到0-1
min_value_normalized = (Z_drdbk[min_pos] - vmin) / (vmax - vmin)  # 归一化到0-1


# 从颜色映射中获取对应颜色
max_color = custom_cmap(max_value_normalized)
min_color = custom_cmap(min_value_normalized)
# 使用对应的颜色绘制点，白色边框
ax.plot(max_pos[1]+0.5, max_pos[0]+0.5, 'o', color=max_color, markersize=8, 
       markeredgecolor='white', markeredgewidth=1.5)
ax.plot(min_pos[1]+0.5, min_pos[0]+0.5, 'o', color=min_color, markersize=8, 
       markeredgecolor='white', markeredgewidth=1.5)
ax.annotate(f'{Z_drdbk[max_pos]:.2f}', xy=(max_pos[1]+0.5, max_pos[0]+0.5), 
           xytext=(-20, -45), textcoords='offset points', 
           color='white', fontweight='bold', backgroundcolor='black', alpha=0.7)
ax.annotate(f'{Z_drdbk[min_pos]:.2f}', xy=(min_pos[1]+0.5, min_pos[0]+0.5), 
           xytext=(10, 10), textcoords='offset points', 
           color='white', fontweight='bold', backgroundcolor='black', alpha=0.7)

plt.tight_layout()  # 调整布局
plt.savefig('figure15.png', bbox_inches='tight')
plt.savefig('figure15.pdf', bbox_inches='tight') 