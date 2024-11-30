import pandas
import matplotlib
import pathlib
from pathlib import Path
from plot_functions import plot_bar_chart
def sortColumns(df):
    seqList = ["lru","foo1","foo2","foo3","foo4","foo5","Belady"]
    df = df[seqList]
    return df


basePath=pathlib.Path("./plot")
baseStatPath=pathlib.Path("./stat")
data = pandas.read_csv("./stat/combine_result.csv")

ipc = data.pivot(index='app', columns='Para3', values='ipc')
ipc = sortColumns(ipc)

plot_bar_chart(output_path=pathlib.Path(basePath/"./ipc.png"),
               df=ipc,
               ytitle="IPC",
               xlabel_rotation=0,
               legend_col_num=10,
               show_legend=True,
               plot_type="bar",
               add_average=False,
               axis_font_size=12,
               figsize=(2, 8),
               show_value_texts = True
               )
ipc.to_csv(baseStatPath/"ipc.csv")
ipc_speed_up = ipc
for col in ipc_speed_up.columns.values:
    if col == 'lru':
        continue
    ipc_speed_up[col] = (ipc_speed_up[col]-ipc_speed_up["lru"])/ipc_speed_up['lru']*100
ipc_speed_up.drop(columns=['lru'],inplace=True)

plot_bar_chart(output_path=basePath/"ipc_speedup.png",
               df=ipc_speed_up,
               ytitle="IPC speedup (\%)",
               xlabel_rotation=0,
               legend_col_num=10,
               show_legend=True,
               plot_type="bar",
               axis_font_size=12,
               figsize=(2, 8),
               show_value_texts = False
               )

ipc_speed_up.to_csv(baseStatPath/"ipc_speedup.csv")
mr = data.pivot(index='app', columns='Para3', values='UOP_MR')
mr = sortColumns(mr)

plot_bar_chart(output_path=pathlib.Path(basePath/"./mr.png"),
               df=mr,
               ytitle="Miss rate",
               xlabel_rotation=0,
               legend_col_num=10,
               show_legend=True,
               plot_type="bar",
               axis_font_size=12,
               figsize=(2, 8),
               show_value_texts = False
               )
mr.to_csv(baseStatPath/"mr.csv")
mr_decrease = mr
for col in mr_decrease.columns.values:
    if col == 'lru':
        continue
    mr_decrease[col] = -(mr_decrease[col]-mr_decrease["lru"])/mr_decrease['lru']*100
mr_decrease.drop(columns=['lru'],inplace=True)

plot_bar_chart(output_path=basePath/"mr_decrease.png",
               df=mr_decrease,
               ytitle="Miss reduce (\%)",
               xlabel_rotation=0,
               legend_col_num=10,
               show_legend=True,
               plot_type="bar",
               axis_font_size=12,
               figsize=(2, 8),
               show_value_texts = False
               )
mr_decrease.to_csv(baseStatPath/"mr_decrease.csv")