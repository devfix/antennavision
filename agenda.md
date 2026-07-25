# Agenda

## Software
- [x] look for setup files in a directory, loop over all setups
- [x] implement plot_gain_over_straight
- [x] implement ULA
- [x] check setup timestamp -> update only if new
- [x] fix cylinder width in three: line2
- [x] remove fmt dependency finally
- [x] add StandingWaveDipole to factory::make_radiator
- [x] implement beamwidth measure function
- [x] pre compute radiation resistance for gain and directivtiy
- [x] add UPA
- [x] add beamforming coefficients
- [x] check angle between axis1 and axis2 at plane definiton -> error if not 90 deg
- [x] parallel plane or better spherical plane
- [x] codebook: different planes with normalized distances to UPA
- [x] added geometries (rectangle, spherical rectangle) as separate definitons in the json and refer to them
- [ ] add tests: UPA gain at different points
- [ ] add tests: UPA gain over spherical rectangle
- [x] rename plot to eval or something similar
- [ ] second definition of spherical rectangle
- [ ] visualize the screens in the 3D view
- [x] CRTP instead of lambdas for the field
- [x] dplot
- [ ] multi-threaded computation
- [ ] ~~implement PS~~
- [ ] implement connections between components
- [ ] low resolution phase shifters
- [ ] find beam areas
- [ ] add spdlog

## Writing
- [ ] derive magnetic vector potential using Lorenz Equation
- [ ] derive effective length vector and express field quantities by it
- [ ] derive beamforming model for PS and TDD
- [ ] derive beam focusing (x,y)
- [ ] E focus vs "P focus" (P variation) -> compare Hertzian Dipole vs lambda/2 dipole