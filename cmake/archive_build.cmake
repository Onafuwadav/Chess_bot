# Copies a freshly built chess_uci.exe to old_builds/chess_uci_vN.exe and logs it in
# old_builds/versions.txt, so every engine version can be re-tested later, e.g.
#   match.ps1 -Name Chess_bot_v3 -Engine old_builds\chess_uci_v3.exe
#
# Run by CMake after chess_uci is linked (see CHESS_ARCHIVE_BUILDS in CMakeLists.txt) with:
#   EXE          the built chess_uci.exe
#   SOURCE_DIR   the repository root
#   ARCHIVE_DIR  where the versions go (old_builds/)
#   GIT          path to git, or empty
#
# Rebuilding the same code gives a slightly different .exe (timestamps, paths), so instead of
# comparing .exe files this fingerprints the source that goes into chess_uci, and only
# archives when that has changed since the last archived version.

# --- Fingerprint the engine and UCI source ---
file(GLOB_RECURSE sources
    "${SOURCE_DIR}/engine/*.cpp" "${SOURCE_DIR}/engine/*.h" "${SOURCE_DIR}/play/uci/*.cpp")
list(SORT sources)
set(listing "")
foreach(source IN LISTS sources)
    file(SHA256 "${source}" hash)
    file(RELATIVE_PATH name "${SOURCE_DIR}" "${source}")
    string(APPEND listing "${name} ${hash}\n")
endforeach()
string(SHA256 fingerprint "${listing}")
string(SUBSTRING "${fingerprint}" 0 12 fingerprint)

# --- Skip if the newest archived version was built from the same source ---
set(log "${ARCHIVE_DIR}/versions.txt")
if(EXISTS "${log}")
    file(STRINGS "${log}" entries REGEX "^v[0-9]+ ")
    list(LENGTH entries count)
    if(count GREATER 0)
        list(GET entries -1 last)
        if(last MATCHES "source ${fingerprint}")
            message(STATUS "chess_uci source unchanged since the last archived version; not archived")
            return()
        endif()
    endif()
endif()

# --- Next version number: highest existing chess_uci_vN.exe + 1 ---
file(MAKE_DIRECTORY "${ARCHIVE_DIR}")
file(GLOB existing "${ARCHIVE_DIR}/chess_uci_v*.exe")
set(version 0)
foreach(path IN LISTS existing)
    get_filename_component(name "${path}" NAME)
    if(name MATCHES "^chess_uci_v([0-9]+)\\.exe$" AND CMAKE_MATCH_1 GREATER version)
        set(version ${CMAKE_MATCH_1})
    endif()
endforeach()
math(EXPR version "${version} + 1")

# --- Which commit it was built from (and whether there were uncommitted changes) ---
set(commit "unknown")
if(GIT)
    execute_process(COMMAND "${GIT}" -C "${SOURCE_DIR}" rev-parse --short HEAD
        OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE git_failed)
    if(git_failed)
        set(commit "unknown")
    else()
        execute_process(COMMAND "${GIT}" -C "${SOURCE_DIR}" status --porcelain -- engine play/uci
            OUTPUT_VARIABLE changes OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(changes)
            string(APPEND commit " +uncommitted changes")
        endif()
    endif()
endif()

# --- Copy and log ---
file(COPY_FILE "${EXE}" "${ARCHIVE_DIR}/chess_uci_v${version}.exe")
string(TIMESTAMP now "%Y-%m-%d %H:%M")
if(NOT EXISTS "${log}")
    file(WRITE "${log}" "# Archived chess_uci builds: version, date, git commit, source fingerprint\n")
endif()
file(APPEND "${log}" "v${version}  ${now}  commit ${commit}  source ${fingerprint}\n")
message(STATUS "Archived chess_uci as old_builds/chess_uci_v${version}.exe (commit ${commit})")
