# AntennaVision

Antenna Simulation Tool

## Agenda

### Features for v1.0.0
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
- [x] add tests: ScalarField gain at different points
- [x] add tests: ScalarField gain over spherical rectangle
- [x] rename plot to eval or something similar
- [x] second definition of spherical rectangle
- [x] visualize the geometries in the 3D view
- [x] CRTP instead of lambdas for the field
- [x] dplot
- [x] multi-threaded computation
- [x] add Isotropic Radiator
- [x] Version check for setup files
- [x] add disable-attenuation-flag to num_params
- [x] commandline parameters for output control
- [ ] implement directivity task for arrays (at the moment only supported for radiators)
- [x] if debug run: print sim params
- [x] Check result file size > 0 OR create result.json as the last step in the eval functions
- [x] print actual coordinates (pos, rot, ex ey ez) for antennas and references
- [ ] eval electrical field
- [ ] ~optional with references~ &rarr; not supported by compiler yet


### Future Features
- [x] add output verbosity control
- [ ] implement UCA
- [ ] print references as table instead of list
- [ ] implement tx voltage field
- [ ] find beam areas
- [ ] find beam area distance

## Writing
- [ ] derive magnetic vector potential using Lorenz Equation
- [ ] derive effective length vector and express field quantities by it
- [ ] derive beamforming model for PS and TDD
- [ ] derive beam focusing (x,y)
- [ ] E focus vs "P focus" (P variation) -> compare Hertzian Dipole vs lambda/2 dipole