import pandas
import numpy

data = pandas.read_csv("./stat/combine_result.csv")
# seqList = ["lru", "transient","holistic","3bit","opt"]
# paraToIndex={k: str(v) for v, k in enumerate(seqList)}
# data["temp"] = data["Para0"]
# data = data.replace({"temp": paraToIndex})
# data["temp"] = data["app"]+"             "+data["temp"]
# data = data.sort_values(by=["temp"])
# data = data.drop(columns="temp")
# print(data.columns.values)
data.to_csv("./stat/sort.csv", index=False)