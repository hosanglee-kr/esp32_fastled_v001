# .github/workflows/ci.yml (또는 해당 파일 이름)
name: Add firmware links to pull request # Pull Request에 펌웨어 링크 추가

on:
  workflow_run:
    workflows: [PR] # 'PR' 워크플로우가 완료되었을 때 트리거
    types: [completed] # 완료 상태일 때 실행

jobs:
  add-artifact-links: # 아티팩트 링크 추가 Job
    runs-on: ubuntu-latest
    # 트리거한 워크플로우(PR)가 성공했을 때만 이 Job 실행
    if: ${{ github.event.workflow_run.conclusion == 'success' }}

    # --- 필요한 권한 명시적으로 부여 ---
    permissions:
      actions: read         # 트리거 워크플로우 실행의 아티팩트 목록을 읽기 위해 필요
      pull-requests: write  # 연결된 Pull Request에 댓글을 달기 위해 필요 (PR은 이슈의 일종)
      issues: write         # Pull Request/이슈에 댓글 쓰기 권한 (pull-requests: write와 함께 사용하거나 대체 가능)
    # ----------------------------------

    steps:
      - name: Get artifacts and generate comment # 아티팩트 목록 가져오고 댓글 내용 생성
        uses: actions/github-script@v6 # GitHub API 호출을 위한 액션 사용
        with:
          github-token: ${{ secrets.GITHUB_TOKEN }} # 기본 제공되는 GitHub 토큰 사용
          script: |
            // 이 워크플로우를 트리거한 워크플로우 실행 ID 가져오기
            const run_id = context.payload.workflow_run.id;
            console.log(`Triggering workflow run ID: ${run_id}`); // 실행 ID 로그 출력

            // 트리거 워크플로우 실행의 아티팩트 목록 가져오기 (댓글 내용 생성을 위해 필요)

            // 'actions: read' 권한 필요
            try {
              const artifacts = await github.rest.actions.listWorkflowRunArtifacts({
                owner: context.repo.owner,
                repo: context.repo.repo,
                run_id: run_id
              });

              // 아티팩트 목록 이름만 나열하는 댓글 내용 생성
              let body = "Firmware artifacts for this pull request:\n"; // 댓글 시작 내용

              if (artifacts.data.artifacts.length > 0) {
                console.log(`Found ${artifacts.data.artifacts.length} artifacts.`); // 찾은 아티팩트 개수 로그

                artifacts.data.artifacts.forEach(artifact => {

                  // 아티팩트 이름만 목록으로 표시 (직접 다운로드 링크 대신)
                  body += `- ${artifact.name}\n`;
                });

                // >>>>> 직접 다운로드 URL 대신 워크플로우 실행 페이지 링크 추가 <<<<<
                // 워크플로우 실행 페이지 URL 생성
                const run_url = `https://github.com/${context.repo.owner}/${context.repo.repo}/actions/runs/${run_id}`;
                // 댓글에 워크플로우 실행 페이지 링크 추가
                body += `\nDownload all artifacts from the workflow run page: [Workflow Run #${run_id}](${run_url})`;
                 console.log(`Workflow Run URL: ${run_url}`); // 워크플로우 실행 URL 로그 출력

              } else {
                 body += "No artifacts found for this run.\n"; // 아티팩트가 없을 경우 메시지
                 console.log("No artifacts found for this run."); // 아티팩트 없음 로그
              }


              // 트리거 워크플로우 실행이 Pull Request와 연결되어 있는지 확인
              // workflow_run은 푸시 등으로도 트리거될 수 있으므로 이 확인이 중요
              if (context.payload.workflow_run.pull_requests && context.payload.workflow_run.pull_requests.length > 0) {
                // 트리거 워크플로우 실행 페이로드에서 PR 번호 가져오기
                const issue_number = context.payload.workflow_run.pull_requests[0].number;
                console.log(`Associated with PR #${issue_number}`); // 연결된 PR 번호 로그

                // 연결된 Pull Request에 댓글 작성
                await github.rest.issues.createComment({
                  owner: context.repo.owner,
                  repo: context.repo.repo,
                  issue_number: issue_number,
                  body: body // 작성할 댓글 내용
                });
                console.log(`Commented on PR #${issue_number}`); // 성공 로그
              } else {
                // PR이 연결되지 않은 경우 (예: 푸시로 트리거) 로그 출력, 댓글 작성 스킵
                console.log("No pull request associated with this workflow run. Skipping comment.");
                console.log("Artifacts found:", artifacts.data.artifacts.map(a => a.name).join(', '));
              }

            } catch (error) {
              // 오류 발생 시 콘솔에 에러 메시지 출력
              console.error("Error listing artifacts or creating comment:", error);
              // 오류를 다시 throw하여 Job이 실패하도록 함
              throw error;
            }
