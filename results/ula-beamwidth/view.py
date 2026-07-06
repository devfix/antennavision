#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import json


def main():
    with open('builtin.t00_compare_beamwidth.json') as fp:
        js = json.load(fp)
    name = js['name']
    distances = np.asarray(js['distances'])
    gains = np.asarray(js['gains'])

    fig, ax = plt.subplots()
    ax.plot(distances, gains, linewidth=2.0)
    #ax.set(xlim=(0, 8), xticks=np.arange(1, 8), ylim=(0, 8), yticks=np.arange(1, 8))

    ax.set_xlabel(r'$z$', fontsize=15)
    ax.set_ylabel(r'$abs{G}$', fontsize=15)
    plt.show()


if __name__ == '__main__':
    main()
