若登入成功會顯示LOGIN SUCCESS 跟 man page
失敗則會顯示LOGIN FAIL!!TERMINATE CLIENT PROCESS 並終止 client程式不占用電腦資源

l    --->列出在線的人
@fd    --->選擇對戰的人
#(0-8)    --->下棋在哪個位子(只有棋局開始才可以使用)
bye    --->下線
/all [mag] --->向所有人發話
/name [msg] --->私訊某人
whoami --->list your name
man --->list manual page

額外處理(期望加分)
1.防呆(對戰方面)
	若一人向多人發起邀請，最先同意的可以進行對戰
	若在棋局中不可邀請別人或受邀請
	若有人上線跟離線會顯示給所有使用者看
	會印出manual page提醒使用者如何操作介面
	有做基本防呆，不可與自己對戰，也不可與不存在的使用者對戰，輸入的棋局位置是否在範圍內，都會告訴client哪裡錯了
	若對戰到一半直接離線棋局會直接取消


2./all [msg]可對所有人傳訊息，有防呆，格式錯誤會提醒client
3./name [msg]可私訊(不可以私訊自己)，有防呆，格式錯誤會提醒client
4.whoami 列出自己姓名 如果使用者忘記自己ID可以查
5.man 列出介面說明書
6.使用者輸入不合法指令都會跳出提醒
7.會列出詳細資訊在Server，讓server好管理client操作