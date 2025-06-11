import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('perf.csv')
df['speedup'] = df.groupby(['approach', 'domain', 'workload'])['time sec'].transform(lambda x: x.iloc[0] / x)

plt.figure(figsize=(10,6))
for key, group in df.groupby(['approach', 'domain', 'workload']):
    approach, domain, workload = key
    if workload == 'contains':
        plt.plot(group['threads'], group['speedup'], label=f"{approach}, {domain}")

plt.plot([1,10], [1,10], '--k', label="Ideal Linear Speedup")
plt.xlabel("Threads")
plt.ylabel("Speedup")
plt.title("Speedup vs Threads (Workload: contains)")
plt.legend()
plt.grid(True)
plt.show()