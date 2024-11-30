import pandas
import matplotlib
import pathlib
from pathlib import Path
from plot_functions import plot_bar_chart

def sortColumns(df):
    orderList = ["lru", "SRRIP", "SHiP++", "Mockingjay", "GHRP", "Thermo", "FURBYS", "Belady", "FLACK"]
    df = df[orderList]
    return df

basePath=pathlib.Path("./plot")
baseStatPath=pathlib.Path("./stat")
data = pandas.read_csv("./stat/combine_result.csv")

data["Para3"] = data["Para3"].replace(
    {
        "srrip": "SRRIP",
        "ship": "SHiP++",
        "ghrp": "GHRP",
        "DynamicBypass": "FURBYS",
        "foo": "FLACK",
        "thermo": "Thermo",
        "mockingjay": "Mockingjay"
    }
)
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


mr = data.pivot(index='app', columns='Para3', values='Cycle_saved')
mr = sortColumns(mr)

plot_bar_chart(output_path=pathlib.Path(basePath/"./cycle.png"),
               df=mr,
               ytitle="Cycle Count",
               xlabel_rotation=0,
               legend_col_num=10,
               show_legend=True,
               plot_type="bar",
               axis_font_size=12,
               figsize=(2, 8),
               show_value_texts = False
               )
mr.to_csv(baseStatPath/"Cycle.csv")

mr = data.pivot(index='app', columns='Para3', values='UOP_UC_TO_IC_SWITCH')
mr = sortColumns(mr)

plot_bar_chart(output_path=pathlib.Path(basePath/"./switch.png"),
               df=mr,
               ytitle="Switch Count",
               xlabel_rotation=0,
               legend_col_num=10,
               show_legend=True,
               plot_type="bar",
               axis_font_size=12,
               figsize=(2, 8),
               show_value_texts = False
               )
mr.to_csv(baseStatPath/"switch.csv")


redirect = data.pivot(index='app', columns='Para3', values='Redirect')
redirect = sortColumns(redirect)
for col in redirect.columns.values:
    if col == 'lru':
        continue
    redirect[col] = -(redirect[col]-redirect["lru"])/redirect['lru']*100
redirect.drop(columns=['lru'],inplace=True)

plot_bar_chart(output_path=basePath/"redirect.png",
               df=redirect,
               ytitle="Reduced Redirect Cycles (\%)",
               xlabel_rotation=0,
               legend_col_num=10,
               show_legend=True,
               plot_type="bar",
               axis_font_size=12,
               figsize=(2, 8),
               show_value_texts = False
               )