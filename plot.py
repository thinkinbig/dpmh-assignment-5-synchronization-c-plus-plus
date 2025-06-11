import pandas as pd
import matplotlib.pyplot as plt

# Read CSV file, skip leading spaces
df = pd.read_csv('perf.csv', skipinitialspace=True)
df['threads'] = df['threads'].astype(int)

# Only plot for a specific workload, e.g., 'contains'
workload_to_plot = 'contains'
plot_data = df[df['workload'] == workload_to_plot]

# 1. 先对同一approach、同一线程数做均值
mean_times = plot_data.groupby(['approach', 'threads'])['time sec'].mean().reset_index()

# 2. 计算speedup
speedup_data = []
for approach in mean_times['approach'].unique():
    approach_data = mean_times[mean_times['approach'] == approach].sort_values('threads')
    single_thread_time = approach_data[approach_data['threads'] == 1]['time sec'].values[0]
    approach_data = approach_data.copy()
    approach_data['speedup'] = single_thread_time / approach_data['time sec']
    speedup_data.append(approach_data)

speedup_df = pd.concat(speedup_data)

# 3. 绘图
plt.figure(figsize=(10, 6))
for approach in speedup_df['approach'].unique():
    approach_data = speedup_df[speedup_df['approach'] == approach]
    plt.plot(approach_data['threads'], approach_data['speedup'], marker='o', label=approach)

# ideal line
threads_sorted = sorted(speedup_df['threads'].unique())
plt.plot(threads_sorted, threads_sorted, 'k--', label='ideal')

plt.title(f'Speedup vs Threads ({workload_to_plot})')
plt.xlabel('Threads')
plt.ylabel('Speedup')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('speedup_vs_threads.png')
plt.close()

# 4. 绘制 time vs threads
plt.figure(figsize=(10, 6))
for approach in mean_times['approach'].unique():
    approach_data = mean_times[mean_times['approach'] == approach]
    plt.plot(approach_data['threads'], approach_data['time sec'], marker='o', label=approach)

plt.title(f'Average Time vs Threads ({workload_to_plot})')
plt.xlabel('Threads')
plt.ylabel('Average Time (sec)')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig('time_vs_threads.png')
plt.close()