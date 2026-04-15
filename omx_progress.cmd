@echo off
omx team api send-message --input "{""team_name"":""tofix-source-plugins-config"",""from_worker"":""worker-3"",""to_worker"":""leader-fixed"",""body"":""Progress: submodule plugins CommonLibraryPlugin/CustomUIPlugin/SaveGamePlugin are empty in this worktree and break current plugin dependency graph; documenting evidence now.""}" --json
