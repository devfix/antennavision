#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import json


def main():
    with open('builtin.t00_compare_beamwidth.result.json') as fp:
        js = json.load(fp)
    name = js['name']
    ns_elements = np.asarray(js['ns_elements'])
    beamwidths_axial = np.degrees(np.asarray(js['beamwidths_axial']))
    beamwidths_lateral = np.degrees(np.asarray(js['beamwidths_lateral']))

    fig, ax = plt.subplots()
    ax.plot(ns_elements, beamwidths_axial, linewidth=1.0, label="axial")
    ax.plot(ns_elements, beamwidths_lateral, linewidth=1.0, label="lateral")
    #ax.set(xlim=(0, 8), xticks=np.arange(1, 8), ylim=(0, 8), yticks=np.arange(1, 8))

    ax.set_xlabel(r'ns_elements', fontsize=15)
    ax.set_ylabel(r'beamwidths', fontsize=15)
    ax.set_xscale('log', base=2)
    plt.show()


if __name__ == '__main__':
    main()
