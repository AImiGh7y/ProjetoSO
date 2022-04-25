import matplotlib
matplotlib.use("agg")
import sys
import pandas as pd
import numpy as np
from plotnine import *

def main():
    fn = sys.argv[1]

    df = pd.read_csv(sys.stdin)

    #print(df) # for debug

    p = ggplot(df) + aes(x="time", y="count") + geom_area(position = "identity") + geom_line() + geom_point()
    p.save(filename = fn + '.png', height=12, width=12, units = 'cm', dpi=300)

if __name__ == '__main__':
    main()
