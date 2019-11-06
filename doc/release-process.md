Release Process
====================

Before every release candidate:

* Update translations (ping wumpus on IRC) see [translation_process.md](https://github.com/readercoin/readercoin/blob/master/doc/translation_process.md#synchronising-translations).

* Update manpages, see [gen-manpages.sh](https://github.com/readercoin/readercoin/blob/master/contrib/devtools/README.md#gen-manpagessh).
* Update release candidate version in `configure.ac` (`CLIENT_VERSION_RC`)

Before every minor and major release:

* Update [bips.md](bips.md) to account for changes since the last release.
* Update version in `configure.ac` (don't forget to set `CLIENT_VERSION_IS_RELEASE` to `true`) (don't forget to set `CLIENT_VERSION_RC` to `0`)
* Write release notes (see below)
* Update `src/chainparams.cpp` nMinimumChainWork with information from the getblockchaininfo rpc.
* Update `src/chainparams.cpp` defaultAssumeValid with information from the getblockhash rpc.
  - The selected value must not be orphaned so it may be useful to set the value two blocks back from the tip.
  - Testnet should be set some tens of thousands back from the tip due to reorgs there.
  - This update should be reviewed with a reindex-chainstate with assumevalid=0 to catch any defect
     that causes rejection of blocks in the past history.

Before every major release:

* Update hardcoded [seeds](/contrib/seeds/README.md), see [this pull request](https://github.com/readercoin/readercoin/pull/7415) for an example.
* Update [`src/chainparams.cpp`](/src/chainparams.cpp) m_assumed_blockchain_size and m_assumed_chain_state_size with the current size plus some overhead.
* Update `src/chainparams.cpp` chainTxData with statistics about the transaction count and rate. Use the output of the RPC `getchaintxstats`, see
  [this pull request](https://github.com/readercoin/readercoin/pull/12270) for an example. Reviewers can verify the results by running `getchaintxstats <window_block_count> <window_last_block_hash>` with the `window_block_count` and `window_last_block_hash` from your output.
* Update version of `contrib/gitian-descriptors/*.yml`: usually one'd want to do this on master after branching off the release - but be sure to at least do it before a new major release

### First time / New builders

If you're using the automated script (found in [contrib/gitian-build.py](/contrib/gitian-build.py)), then at this point you should run it with the "--setup" command. Otherwise ignore this.

Check out the source code in the following directory hierarchy.

    cd /path/to/your/toplevel/build
    git clone https://github.com/readercoin-core/gitian.sigs.git
    git clone https://github.com/readercoin-core/readercoin-detached-sigs.git
    git clone https://github.com/devrandom/gitian-builder.git
    git clone https://github.com/readercoin/readercoin.git

### Readercoin maintainers/release engineers, suggestion for writing release notes

Write release notes. git shortlog helps a lot, for example:

    git shortlog --no-merges v(current version, e.g. 0.7.2)..v(new version, e.g. 0.8.0)

(or ping @wumpus on IRC, he has specific tooling to generate the list of merged pulls
and sort them into categories based on labels)

Generate list of authors:

    git log --format='- %aN' v(current version, e.g. 0.16.0)..v(new version, e.g. 0.16.1) | sort -fiu

Tag version (or release candidate) in git

    git tag -s v(new version, e.g. 0.8.0)

### Setup and perform Gitian builds

If you're using the automated script (found in [contrib/gitian-build.py](/contrib/gitian-build.py)), then at this point you should run it with the "--build" command. Otherwise ignore this.

Setup Gitian descriptors:

    pushd ./readercoin
    export SIGNER="(your Gitian key, ie bluematt, sipa, etc)"
    export VERSION=(new version, e.g. 0.8.0)
    git fetch
    git checkout v${VERSION}
    popd

Ensure your gitian.sigs are up-to-date if you wish to gverify your builds against other Gitian signatures.

    pushd ./gitian.sigs
    git pull
    popd

Ensure gitian-builder is up-to-date:

    pushd ./gitian-builder
    git pull
    popd

### Fetch and create inputs: (first time, or when dependency versions change)

    pushd ./gitian-builder
    mkdir -p inputs
    wget -P inputs https://readercoincore.org/cfields/osslsigncode-Backports-to-1.7.1.patch
    echo 'a8c4e9cafba922f89de0df1f2152e7be286aba73f78505169bc351a7938dd911 inputs/osslsigncode-Backports-to-1.7.1.patch' | sha256sum -c
    wget -P inputs https://downloads.sourceforge.net/project/osslsigncode/osslsigncode/osslsigncode-1.7.1.tar.gz
    echo 'f9a8cdb38b9c309326764ebc937cba1523a3a751a7ab05df3ecc99d18ae466c9 inputs/osslsigncode-1.7.1.tar.gz' | sha256sum -c
    popd

Create the macOS SDK tarball, see the [macOS build instructions](build-osx.md#deterministic-macos-dmg-notes) for details, and copy it into the inputs directory.

### Optional: Seed the Gitian sources cache and offline git repositories

NOTE: Gitian is sometimes unable to download files. If you have errors, try the step below.

By default, Gitian will fetch source files as needed. To cache them ahead of time, make sure you have checked out the tag you want to build in readercoin, then:

    pushd ./gitian-builder
    make -C ../readercoin/depends download SOURCES_PATH=`pwd`/cache/common
    popd

Only missing files will be fetched, so this is safe to re-run for each build.

NOTE: Offline builds must use the --url flag to ensure Gitian fetches only from local URLs. For example:

    pushd ./gitian-builder
    ./bin/gbuild --url readercoin=/path/to/readercoin,signature=/path/to/sigs {rest of arguments}
    popd

The gbuild invocations below <b>DO NOT DO THIS</b> by default.

### Build and sign Readercoin Core for Linux, Windows, and macOS:

    pushd ./gitian-builder
    ./bin/gbuild --num-make 2 --memory 3000 --commit readercoin=v${VERSION} ../readercoin/contrib/gitian-descriptors/gitian-linux.yml
    ./bin/gsign --signer "$SIGNER" --release ${VERSION}-linux --destination ../gitian.sigs/ ../readercoin/contrib/gitian-descriptors/gitian-linux.yml
    mv build/out/readercoin-*.tar.gz build/out/src/readercoin-*.tar.gz ../

    ./bin/gbuild --num-make 2 --memory 3000 --commit readercoin=v${VERSION} ../readercoin/contrib/gitian-descriptors/gitian-win.yml
    ./bin/gsign --signer "$SIGNER" --release ${VERSION}-win-unsigned --destination ../gitian.sigs/ ../readercoin/contrib/gitian-descriptors/gitian-win.yml
    mv build/out/readercoin-*-win-unsigned.tar.gz inputs/readercoin-win-unsigned.tar.gz
    mv build/out/readercoin-*.zip build/out/readercoin-*.exe ../

    ./bin/gbuild --num-make 2 --memory 3000 --commit readercoin=v${VERSION} ../readercoin/contrib/gitian-descriptors/gitian-osx.yml
    ./bin/gsign --signer "$SIGNER" --release ${VERSION}-osx-unsigned --destination ../gitian.sigs/ ../readercoin/contrib/gitian-descriptors/gitian-osx.yml
    mv build/out/readercoin-*-osx-unsigned.tar.gz inputs/readercoin-osx-unsigned.tar.gz
    mv build/out/readercoin-*.tar.gz build/out/readercoin-*.dmg ../
    popd

Build output expected:

  1. source tarball (`readercoin-${VERSION}.tar.gz`)
  2. linux 32-bit and 64-bit dist tarballs (`readercoin-${VERSION}-linux[32|64].tar.gz`)
  3. windows 32-bit and 64-bit unsigned installers and dist zips (`readercoin-${VERSION}-win[32|64]-setup-unsigned.exe`, `readercoin-${VERSION}-win[32|64].zip`)
  4. macOS unsigned installer and dist tarball (`readercoin-${VERSION}-osx-unsigned.dmg`, `readercoin-${VERSION}-osx64.tar.gz`)
  5. Gitian signatures (in `gitian.sigs/${VERSION}-<linux|{win,osx}-unsigned>/(your Gitian key)/`)

### Verify other gitian builders signatures to your own. (Optional)

Add other gitian builders keys to your gpg keyring, and/or refresh keys: See `../readercoin/contrib/gitian-keys/README.md`.

Verify the signatures

    pushd ./gitian-builder
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-linux ../readercoin/contrib/gitian-descriptors/gitian-linux.yml
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-win-unsigned ../readercoin/contrib/gitian-descriptors/gitian-win.yml
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-unsigned ../readercoin/contrib/gitian-descriptors/gitian-osx.yml
    popd

### Next steps:

Commit your signature to gitian.sigs:

    pushd gitian.sigs
    git add ${VERSION}-linux/"${SIGNER}"
    git add ${VERSION}-win-unsigned/"${SIGNER}"
    git add ${VERSION}-osx-unsigned/"${SIGNER}"
    git commit -m "Add ${VERSION} unsigned sigs for ${SIGNER}"
    git push  # Assuming you can push to the gitian.sigs tree
    popd

Codesigner only: Create Windows/macOS detached signatures:
- Only one person handles codesigning. Everyone else should skip to the next step.
- Only once the Windows/macOS builds each have 3 matching signatures may they be signed with their respective release keys.

Codesigner only: Sign the macOS binary:

    transfer readercoin-osx-unsigned.tar.gz to macOS for signing
    tar xf readercoin-osx-unsigned.tar.gz
    ./detached-sig-create.sh -s "Key ID"
    Enter the keychain password and authorize the signature
    Move signature-osx.tar.gz back to the gitian host

Codesigner only: Sign the windows binaries:

    tar xf readercoin-win-unsigned.tar.gz
    ./detached-sig-create.sh -key /path/to/codesign.key
    Enter the passphrase for the key when prompted
    signature-win.tar.gz will be created

Codesigner only: Commit the detached codesign payloads:

    cd ~/readercoin-detached-sigs
    checkout the appropriate branch for this release series
    rm -rf *
    tar xf signature-osx.tar.gz
    tar xf signature-win.tar.gz
    git add -a
    git commit -m "point to ${VERSION}"
    git tag -s v${VERSION} HEAD
    git push the current branch and new tag

Non-codesigners: wait for Windows/macOS detached signatures:

- Once the Windows/macOS builds each have 3 matching signatures, they will be signed with their respective release keys.
- Detached signatures will then be committed to the [readercoin-detached-sigs](https://github.com/readercoin-core/readercoin-detached-sigs) repository, which can be combined with the unsigned apps to create signed binaries.

Create (and optionally verify) the signed macOS binary:

    pushd ./gitian-builder
    ./bin/gbuild -i --commit signature=v${VERSION} ../readercoin/contrib/gitian-descriptors/gitian-osx-signer.yml
    ./bin/gsign --signer "$SIGNER" --release ${VERSION}-osx-signed --destination ../gitian.sigs/ ../readercoin/contrib/gitian-descriptors/gitian-osx-signer.yml
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-signed ../readercoin/contrib/gitian-descriptors/gitian-osx-signer.yml
    mv build/out/readercoin-osx-signed.dmg ../readercoin-${VERSION}-osx.dmg
    popd

Create (and optionally verify) the signed Windows binaries:

    pushd ./gitian-builder
    ./bin/gbuild -i --commit signature=v${VERSION} ../readercoin/contrib/gitian-descriptors/gitian-win-signer.yml
    ./bin/gsign --signer "$SIGNER" --release ${VERSION}-win-signed --destination ../gitian.sigs/ ../readercoin/contrib/gitian-descriptors/gitian-win-signer.yml
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-win-signed ../readercoin/contrib/gitian-descriptors/gitian-win-signer.yml
    mv build/out/readercoin-*win64-setup.exe ../readercoin-${VERSION}-win64-setup.exe
    mv build/out/readercoin-*win32-setup.exe ../readercoin-${VERSION}-win32-setup.exe
    popd

Commit your signature for the signed macOS/Windows binaries:

    pushd gitian.sigs
    git add ${VERSION}-osx-signed/"${SIGNER}"
    git add ${VERSION}-win-signed/"${SIGNER}"
    git commit -a
    git push  # Assuming you can push to the gitian.sigs tree
    popd

### After 3 or more people have gitian-built and their results match:

- Create `SHA256SUMS.asc` for the builds, and GPG-sign it:

```bash
sha256sum * > SHA256SUMS
```

The list of files should be:
```
readercoin-${VERSION}-aarch64-linux-gnu.tar.gz
readercoin-${VERSION}-arm-linux-gnueabihf.tar.gz
readercoin-${VERSION}-i686-pc-linux-gnu.tar.gz
readercoin-${VERSION}-x86_64-linux-gnu.tar.gz
readercoin-${VERSION}-osx64.tar.gz
readercoin-${VERSION}-osx.dmg
readercoin-${VERSION}.tar.gz
readercoin-${VERSION}-win32-setup.exe
readercoin-${VERSION}-win32.zip
readercoin-${VERSION}-win64-setup.exe
readercoin-${VERSION}-win64.zip
```
The `*-debug*` files generated by the gitian build contain debug symbols
for troubleshooting by developers. It is assumed that anyone that is interested
in debugging can run gitian to generate the files for themselves. To avoid
end-user confusion about which file to pick, as well as save storage
space *do not upload these to the readercoin.org server, nor put them in the torrent*.

- GPG-sign it, delete the unsigned file:
```
gpg --digest-algo sha256 --clearsign SHA256SUMS # outputs SHA256SUMS.asc
rm SHA256SUMS
```
(the digest algorithm is forced to sha256 to avoid confusion of the `Hash:` header that GPG adds with the SHA256 used for the files)
Note: check that SHA256SUMS itself doesn't end up in SHA256SUMS, which is a spurious/nonsensical entry.

- Upload zips and installers, as well as `SHA256SUMS.asc` from last step, to the readercoin.org server
  into `/var/www/bin/readercoin-core-${VERSION}`

- A `.torrent` will appear in the directory after a few minutes. Optionally help seed this torrent. To get the `magnet:` URI use:
```bash
transmission-show -m <torrent file>
```
Insert the magnet URI into the announcement sent to mailing lists. This permits
people without access to `readercoin.org` to download the binary distribution.
Also put it into the `optional_magnetlink:` slot in the YAML file for
readercoin.org (see below for readercoin.org update instructions).

- Update readercoin.org version

  - First, check to see if the Readercoin.org maintainers have prepared a
    release: https://github.com/readercoin-dot-org/readercoin.org/labels/Core

      - If they have, it will have previously failed their Travis CI
        checks because the final release files weren't uploaded.
        Trigger a Travis CI rebuild---if it passes, merge.

  - If they have not prepared a release, follow the Readercoin.org release
    instructions: https://github.com/readercoin-dot-org/readercoin.org/blob/master/docs/adding-events-release-notes-and-alerts.md#release-notes

  - After the pull request is merged, the website will automatically show the newest version within 15 minutes, as well
    as update the OS download links. Ping @saivann/@harding (saivann/harding on Freenode) in case anything goes wrong

- Update other repositories and websites for new version

  - readercoincore.org blog post

  - readercoincore.org RPC documentation update

  - Update packaging repo

      - Notify BlueMatt so that he can start building [the PPAs](https://launchpad.net/~readercoin/+archive/ubuntu/readercoin)

      - Create a new branch for the major release "0.xx" (used to build the snap package)

      - Notify MarcoFalke so that he can start building the snap package

        - https://code.launchpad.net/~readercoin-core/readercoin-core-snap/+git/packaging (Click "Import Now" to fetch the branch)
        - https://code.launchpad.net/~readercoin-core/readercoin-core-snap/+git/packaging/+ref/0.xx (Click "Create snap package")
        - Name it "readercoin-core-snap-0.xx"
        - Leave owner and series as-is
        - Select architectures that are compiled via gitian
        - Leave "automatically build when branch changes" unticked
        - Tick "automatically upload to store"
        - Put "readercoin-core" in the registered store package name field
        - Tick the "edge" box
        - Put "0.xx" in the track field
        - Click "create snap package"
        - Click "Request builds" for every new release on this branch (after updating the snapcraft.yml in the branch to reflect the latest gitian results)
        - Promote release on https://snapcraft.io/readercoin-core/releases if it passes sanity checks

  - This repo

      - Archive release notes for the new version to `doc/release-notes/` (branch `master` and branch of the release)

      - Create a [new GitHub release](https://github.com/readercoin/readercoin/releases/new) with a link to the archived release notes.

- Announce the release:

  - readercoin-dev and readercoin-core-dev mailing list

  - Readercoin Core announcements list https://readercoincore.org/en/list/announcements/join/

  - Update title of #readercoin on Freenode IRC

  - Optionally twitter, reddit /r/Readercoin, ... but this will usually sort out itself

  - Celebrate
