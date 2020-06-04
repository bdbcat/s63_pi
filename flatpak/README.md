s63 flatpak README
---------------------

This is the flatpak packaging of the s63, to be managed
by opencpn's new plugin installer.


Testing
-------
  - The plugin requires extended permissions. Do (initial setup):

      $ flatpak override --user --allow=devel

  - Install flatpak and flatpak-builder as described in https://flatpak.org/
  - Enable the flathub repo and install platform packages:
     
      $  flatpak --user remote-add --if-not-exists \
            flathub https://dl.flathub.org/repo/flathub.flatpakrepo
      $ flatpak --user install org.freedesktop.Platform//18.08
      $ flatpak --user install org.freedesktop.Sdk//18.08
      $ flatpak --user install org.flatpak.Builder

  - Install opencpn from the beta testing repo:

      $ flatpak --user remote-add --no-gpg-verify plug-mgr \
           http://opencpn.duckdns.org/opencpn-beta/website/repo
      $ flatpak --user install plug-mgr org.opencpn.OpenCPN

  - Build plugin tarball and metadata from the ci branch:

      $ cd build
      $ cmake -DOCPN_FLATPAK=ON ..
      $ make flatpak-build
      $ make package
