#!/usr/bin/env fish

set -l script_dir (path dirname (status filename))
set -l project_dir $script_dir

jq -c '.[]' "$project_dir/compile_commands.json" | while read -l entry
    set -l directory (echo $entry | jq -r '.directory')
    set -l file (echo $entry | jq -r '.file')
    set -l command (echo $entry | jq -r '.command')

    cd $directory
    sh -c "$command -fsyntax-only" >/dev/null 2>&1

    if test $status -ne 0
        realpath --relative-to="$script_dir" "$file"
    end

    cd $project_dir
end
