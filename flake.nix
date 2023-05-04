{
  # Copied from github:dpaetzel/nixos-config's inputs.nixpkgs.
  inputs.nixpkgs.url =
      "github:NixOS/nixpkgs/1e2590679d0ed2cee2736e8b80373178d085d263";

  inputs.dSFMTSource.url = "github:MersenneTwister-Lab/dSFMT";
  inputs.dSFMTSource.flake = false;

  outputs = { self, nixpkgs, dSFMTSource }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      python = pkgs.python310;

      # XCSF's build process expects the dSFMT library source to be available.
      # Therefore, we make it available.
      dSFMT = pkgs.stdenv.mkDerivation {
        name = "dSFMT";
        version = "dev";

        src = dSFMTSource;
        dontConfigure = true;
        dontBuild = true;
        installPhase = ''
          mkdir "$out"
          cp -r * "$out"
        '';
      };

      xcsf =
        python.pkgs.toPythonModule (pkgs.stdenv.mkDerivation rec {
          enablePython = true;
          pythonPackages = python;

          pname = "xcsf";
          version = "1.3.1";

          src = self;

          patchPhase = ''
            sed -i 's|add_subdirectory(lib/pybind11)|find_package(pybind11 CONFIG)|' \
                CMakeLists.txt
            sed -i 's|''${CMAKE_SOURCE_DIR}/lib/dSFMT|${dSFMT}|' \
                xcsf/CMakeLists.txt
            sed -i 's|''${CMAKE_SOURCE_DIR}/lib/cJSON|${pkgs.cjson.src}|' \
                xcsf/CMakeLists.txt
            sed -i 's|../lib/cJSON|${pkgs.cjson.src}|' xcsf/utils.h
            sed -i 's|../lib/dSFMT|${dSFMT}|' xcsf/utils.h
            sed -i 's|../lib/doctest|${pkgs.doctest.src}|' test/*.cpp
          '';

          nativeBuildInputs = [
            pkgs.extra-cmake-modules
            python.pkgs.pybind11
          ];

          propagatedBuildInputs = [
            python.pkgs.numpy
          ];

          buildInputs = [
            pkgs.doxygen
            pkgs.graphviz
            python
          ];
          # TODO Add openmp?

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=RELEASE"

            "-DXCSF_PYLIB=ON"
            "-DENABLE_TESTS=ON"
            "-DPARALLEL=ON"
          ];

          installPhase = ''
            mkdir -p $out/${python.sitePackages}
            cp xcsf/xcsf.cpython-310-x86_64-linux-gnu.so $out/${python.sitePackages}
          '';

          meta = with pkgs.lib; {
            description = "Implementation of the XCSF learning classifier system";
            longDescription = ''
              Preen's Python bindings for his implementation of the XCSF learning
              classifier system in C.
            '';
            homepage = "https://github.com/rpreen/xcsf";
            license = licenses.gpl3;
            maintainers = [ maintainers.dpaetzel ];
            platforms = platforms.all;
          };
        });

    in {
      defaultPackage.${system} = xcsf;

      # You can test this by doing `nix develop path:path/to/this/repo`. Make
      # sure to change directory beforehand so Python doesn't attempt to import
      # from this repository's `xcsf` folder.
      devShell.${system} = pkgs.mkShell {
        buildInputs = [
          (python.withPackages(ps: [xcsf]))
        ];

        shellHook = ''
          echo "WARNING: Make sure to exit the repository's directory before"\
               "entering the dev shell as otherwise Python will try to import"\
               "local files."
          python -c 'import xcsf; xcsf.XCS(max_trials=1000).fit([[1]], [[1]])'\
               && echo 'It works!'
        '';
      };
    };
}
