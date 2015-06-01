#ifndef _NUPT_COMMUNICATION_H
#define _NUPT_COMMUNICATION_H
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 6001
#define BUFLEN 1024

struct Seat{
	std::string role;//big/small blind
	std::string pid;
	std::string jetton;
	std::string money;
};

struct Blind{
	std::string pid;
	std::string bet;
};

struct Card{
	std::string color;
	std::string point;
};

struct PlayerStatus{
	std::string pid;
	std::string jetton;
	std::string money;
	std::string bet;
	std::string action;
};

struct Inquire{
	typedef struct PlayerStatus Status;
	std::vector<Status> allStatus; 
	std::string potnum;
};

struct Rank{
	typedef struct Card Card;
	std::string pid;
	Card card1;
	Card card2;
	std::string nut_hand;
};

struct ShowDown{
	typedef struct Card Common;
	typedef struct Rank Rank;
	std::vector<Common> commons;
	std::vector<Rank> ranks;
};

struct Pot{
	std::string pid;
	std::string num;
};

enum InfoType{
	UNKNOWN,SEATINFO,GAMEOVER,BLINDMSG,HOLDCARDS,INQUIREMSG,
	FLOPMSG,TURNMSG,RIVERMSG,SHOWDOWN,POTWIN,NOTIFY
};

enum Color{
	SPADES,HEARTS,CLUBS,DIAMONDS
};

enum NutHand{
	HIGH_CARD,ONE_PAIR,TWO_PAIR,THREE_OF_A_KIND,STRAIGHT,FLUSH,
	FULL_HOUSE,FOUR_OF_A_KIND,STRAIGHT_FLUSH
};

enum RoundStatus{
	OTHERS,PREFLOPROUND,FLOPROUND,TURNROUND,RIVERROUND
};

struct Action{
	static const std::string CHECK;
	static const std::string CALL;//
	static const std::string RAISE;
	static const std::string ALL_IN;
	static const std::string FOLD;
};
const std::string Action::CHECK = "check";
const std::string Action::CALL = "call";
const std::string Action::RAISE = "raise";
const std::string Action::ALL_IN = "all_in";
const std::string Action::FOLD = "fold";

class Player{
public:
	typedef struct sockaddr_in sockaddr_in;
	typedef struct sockaddr sockaddr;
	typedef struct Seat Seat;
	typedef struct Blind Blind;
	typedef struct Card Holdcard;
	typedef struct Card Flop;
	typedef struct Card Turn;
	typedef struct Card River;
	typedef struct Card Common;
	typedef struct PlayerStatus Status;
	typedef struct Inquire Inquire;
	typedef struct Rank Rank;
	typedef struct ShowDown ShowDown;
	typedef struct Pot Pot;
	typedef Inquire Notify;
	typedef enum InfoType InfoType;
	typedef enum RoundStatus RoundStatus;
	typedef enum Color Color;
	typedef struct Action Action;
public:
	Player(std::string pid,std::string pname);
	void bind(int cliport,const char *cliaddr);
	int start(int servport,const char *servaddr);
	void over();
public:
	int regist();
	void recvmsg();
	InfoType infotype();
	void handleInfo(InfoType);
	std::string getRecvinfo(){return buf;}
	std::vector<Seat> getSeatInfo();
	std::vector<Blind> getBlindInfo();
	std::vector<Holdcard> getHoldCards();
	Inquire getInquire();
	int sendAction(const std::string& action);
	std::vector<Flop> getFlopMsg();
	Turn getTurnMsg();
	River getRiverMsg();
	ShowDown getShowDown();
	std::vector<Pot> getPotWin();
	Notify getNotifyMsg();
	Color getColor(std::string& color);
public:
	void seatHandle();
	void gameoverHandle();
	void blindHandle();
	void holdcardHandle();
	void inquireHandle();
	void flopHandle();
	void turnHandle();
	void riverHandle();
	void showdownHandle();
	void potwinHandle();
	void notifyHandle();
public:
	void preFlopAction();
	void flopAction();
	void turnAction();
	void riverAction();
	double cardPower(std::vector<Holdcard> myhc,std::vector<Flop> flops);
private:
	int sockfd;
	sockaddr_in servaddr;
	sockaddr_in cliaddr;
	std::string pid;//player id
	std::string pname;//player name
	std::string buf;//recv info
	std::string recvInfo;//
	RoundStatus myrs;
	std::vector<Seat> seats;
	Seat myseat;
	std::vector<Blind> blinds;
	std::vector<Holdcard> myhc;//2 handcards
	Inquire inquire;
	std::vector<Flop> flops;
	Turn turnCard;
	River riverCard;
	ShowDown sdInfo;
	std::vector<Pot> mypot;
	Notify notifyInfo;
private:
	void putInfo(std::string& strInfo);
};

Player::Player(std::string pid,std::string pname){
	this->pid = pid;
	this->pname = pname;
	this->myrs = OTHERS;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	int reuse = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));
}

void Player::bind(int cliport,const char *addr){
	// extern int bind(int,const sockaddr *,socklen_t);
	this->cliaddr.sin_family = AF_INET;
	this->cliaddr.sin_port = ::htons(cliport);
	this->cliaddr.sin_addr.s_addr = ::inet_addr(addr);
	::bind(sockfd,(sockaddr *)&cliaddr,sizeof(cliaddr));
}

int Player::start(int servport,const char *addr){
	this->servaddr.sin_family = AF_INET;
	this->servaddr.sin_port = ::htons(servport);
	this->servaddr.sin_addr.s_addr = ::inet_addr(addr);
	while (connect(sockfd,(sockaddr *)&servaddr,sizeof(servaddr)) == -1);
	return 0;
}

void Player::over(){
	close(sockfd);
}

int Player::regist(){
	std::string msg;
	msg += "reg: "+pid+" "+pname+" \n";
	const char *tmp = msg.c_str();
	//send register message
	if (send(sockfd,tmp,strlen(tmp),0) == 0)
		return 0;
	else
		return -1;
}

void Player::recvmsg(){ 
	char firstbuf[BUFLEN] = {'\0'};	
	int n = -1;
	if((n = recv(sockfd,firstbuf,BUFLEN,0))<=0)
	{
		std::cout<<"recv failed!"<<std::endl;
		return;
	}
	std::string tempstr(firstbuf);
	recvInfo += tempstr;
	putInfo(recvInfo);	
}

void Player::putInfo(std::string& strInfo)
{
	buf.clear();
	int pos = strInfo.find("\n",0);
	if(pos == -1){ return; }
	std::string strsub = strInfo.substr(0,pos-2);
	int siz = strsub.size();
	pos = strInfo.find(strsub,pos+1);
	strsub = strInfo.substr(0,pos+siz+2);
	if(pos == -1)
	{
		if(strsub == "game-over")
		{
			buf = strsub + " \n";
			strInfo = strInfo.substr(pos+siz+4);
		}
		else
		{
			return;
		}
	}
	else
	{
		buf = strsub;
		strInfo = strInfo.substr(pos+siz+2);
	}
}

Player::InfoType Player::infotype(){
	typedef std::string::size_type size_type;
	std::string msg(buf);
	size_type pos = msg.find_first_of(" ");
	InfoType tp = UNKNOWN;
	if (pos == std::string::npos){
		std::cout<<"info type: unknown"<<std::endl;
		return tp;
	}
	std::string type = msg.substr(0,pos);
	if (type != "game-over")
		type = msg.substr(0,pos-1);
	std::cout<<"info type: "<<type<<std::endl;
	if (type == "seat")
		tp = SEATINFO;
	if (type == "game-over")
		tp = GAMEOVER;
	if (type == "blind")
		tp = BLINDMSG;
	if (type == "hold")
		tp = HOLDCARDS;
	if (type == "inquire")
		tp = INQUIREMSG;
	if (type == "flop")
		tp = FLOPMSG;
	if (type == "turn")
		tp = TURNMSG;
	if (type == "river")
		tp = RIVERMSG;
	if (type == "showdown")
		tp = SHOWDOWN;
	if (type == "pot-win")
		tp = POTWIN;
	if (type == "notify")
		tp = NOTIFY;
	return tp;
}


void Player::seatHandle(){
	std::cout<<">>Enter seatHandle..."<<std::endl;
	seats = getSeatInfo();
	for (int i=0;i<seats.size();++i){
		Seat seat = seats[i];
		std::cout<<"role: "<<seat.role<<" pid: "<<seat.pid;
		std::cout<<" jetton: "<<seat.jetton<<" money: "<<seat.money<<std::endl;
	}

}

void Player::gameoverHandle(){
	std::cout<<">>Enter gameoverHandle..."<<std::endl;
}

void Player::blindHandle(){
	std::cout<<">>Enter blindHandle..."<<std::endl;
	blinds = getBlindInfo();
	for (int i=0;i<blinds.size();++i){
		Blind blind = blinds[i];
		std::cout<<"pid: "<<blind.pid<<" bet: "<<blind.bet<<std::endl;
	}
}

void Player::holdcardHandle(){
	std::cout<<">>Enter holdcardHandle..."<<std::endl;
	myrs = PREFLOPROUND;
	myhc = getHoldCards();
	for (int i=0;i<myhc.size();++i){
		Holdcard hc = myhc[i];
		std::cout<<"color: "<<hc.color<<" point: "<<hc.point<<std::endl;
	}
}


void Player::inquireHandle(){
	std::cout<<">>Enter inquireHandle..."<<std::endl;
	Inquire iq = getInquire();
	std::vector<Status> allStatus = iq.allStatus;
	for (int i=0;i<allStatus.size();++i){
		Status sta = allStatus[i];
		std::cout<<"pid: "<<sta.pid<<" jetton: "<<sta.jetton;
		std::cout<<" money: "<<sta.money<<" bet: "<<sta.bet;
		std::cout<<" action: "<<sta.action<<std::endl;
	}
	std::cout<<" total pot: "<<iq.potnum<<std::endl;
	if (myrs == PREFLOPROUND ){
		//preflop round with 2 handcards
		//Based on Bill Chen Formula to caculate HoldCards' score
		preFlopAction();
	}else if (myrs == FLOPROUND){
		//flop round with 3 public cards on board
		flopAction();
	}else if (myrs == TURNROUND){
		//turn round with 1 turn card added to board
		turnAction();
	}else if (myrs == RIVERROUND){
		//river round with 1 river card added to board
		riverAction();
	}else{
		return ;
	}
}

void Player::flopHandle(){
	std::cout<<">>Enter flopHandle..."<<std::endl;
	myrs = FLOPROUND;
	flops = getFlopMsg();
	for (int i=0;i<flops.size();++i){
		Flop fp = flops[i];
		std::cout<<"color: "<<fp.color<<" point: "<<fp.point<<std::endl;
	}
}

void Player::turnHandle(){
	std::cout<<">>Enter turnHandle..."<<std::endl;
	myrs = TURNROUND;
	turnCard = getTurnMsg();
	std::cout<<"color: "<<turnCard.color<<" point: "<<turnCard.point<<std::endl;
}

void Player::riverHandle(){
	std::cout<<">>Enter riverHandle..."<<std::endl;
	myrs = RIVERROUND;
	riverCard = getRiverMsg();
	std::cout<<"color: "<<riverCard.color<<" point: "<<riverCard.point<<std::endl;
}

void Player::showdownHandle(){
	std::cout<<">>Enter showdownHandle..."<<std::endl;
	sdInfo = getShowDown();
	std::vector<Common> commons = sdInfo.commons;
	for (int i=0;i<commons.size();++i){
		Common cmn = commons[i];
		std::cout<<"color: "<<cmn.color<<" point: "<<cmn.point<<std::endl;
	}
	std::vector<Rank> ranks = sdInfo.ranks;
	for (int i=0;i<ranks.size();++i){
		Rank rank = ranks[i];
		std::cout<<"pid: "<<rank.pid<<" card1: "<<rank.card1.color<<" "<<rank.card1.point;
		std::cout<<" card2: "<<rank.card2.color<<" "<<rank.card2.point<<" nut-hand: "<<rank.nut_hand<<std::endl;
	}
}

void Player::potwinHandle(){
	std::cout<<">>Enter potwinHandle..."<<std::endl;
	mypot = getPotWin();
	for (int i=0;i<mypot.size();++i){
		Pot pot = mypot[i];
		std::cout<<"pid: "<<pot.pid<<" num: "<<pot.num<<std::endl;
	}
}


void Player::notifyHandle(){
	inquireHandle();
}

void Player::handleInfo(InfoType type){
//	pthread_t tid;
	switch(type){
		case SEATINFO:
			seatHandle();
			break;
		case GAMEOVER:
			gameoverHandle();
			break;
		case BLINDMSG:
			blindHandle();
			break;
		case HOLDCARDS:
			holdcardHandle();
			break;
		case INQUIREMSG:
			inquireHandle();
			break;
		case FLOPMSG:
			flopHandle();
			break;
		case TURNMSG:
			turnHandle();
			break;
		case RIVERMSG:
			riverHandle();
			break;
		case SHOWDOWN:
			showdownHandle();
			break;
		case POTWIN:
			potwinHandle();
			break;
		case NOTIFY:
			notifyHandle();
			break;
		default:
			break;
	}
}

/*
* Utility function
*/
/*******************************************************/
std::vector<std::string> splitLine(const std::string& txt){
	std::cout<<">>Enter splitLine..."<<std::endl;
	std::vector<std::string> svec;
	int start = 0;
	int pos = 0;
	int siz = txt.size();
	while (start <= siz-1){
		pos = txt.find_first_of("\n",start);
		if (pos == std::string::npos)
			break;
		std::string str = txt.substr(start,pos-start);
		svec.push_back(str);
		start = pos+1;
	}
	return svec;
}

std::vector<std::string> splitWhiteSpace(const std::string& txt){
	std::vector<std::string> svec;
	int start = 0;
	int pos = 0;
	int siz = txt.size();
	while(start <= siz-1){
		pos = txt.find_first_of(" ",start);
		std::string str = txt.substr(start,pos-start);
		svec.push_back(str);
		start = pos+1;
	}
	return svec;
}

std::string trim(std::string& str){
	int pos1 = 0;
	int pos2 = str.size()-1;
	for (int i=0;i<str.size();++i)
		if (str[i] == ' ')
			++pos1;
		else
			break;
	for (int i = str.size()-1;i>=0;--i)
		if (str[i] == ' ')
			--pos2;
		else
			break;
	if (pos1 == 0 && pos2 == str.size()-1)
		return str;
	else
		return str.substr(pos1,pos2-pos1+1);
}

int strtoint(std::string& str){
	int res = 0;
	std::string tmp = trim(str);
	return atoi(tmp.c_str());
}


/*****************************************************/

std::vector<Player::Seat> Player::getSeatInfo(){
	std::cout<<">>Enter getSeatInfo..."<<std::endl;
	typedef std::string::size_type size_type;
	std::vector<Seat> svec;
	std::string str;
	str += buf;
	std::vector<std::string> lines = splitLine(str);
//	return svec;
	for (int i=1;i<=lines.size()-2;++i){
		std::string line = lines[i];
		size_type pos = line.find_first_of(":",0);
		if (pos == std::string::npos){
			//ordinary player
			Seat tmp;
			std::vector<std::string> words = splitWhiteSpace(line);
			tmp.role = "NONE";
			tmp.pid = words[0];
			tmp.jetton = words[1];
			tmp.money = words[2];
			svec.push_back(tmp);
			if (words[0] == this->pid)
				this->myseat = tmp;
			continue;
		}
		//button or blind
		Seat tmp;
		tmp.role = line.substr(0,pos);
		std::vector<std::string> words = splitWhiteSpace(line.substr(pos+2,line.size()-pos-2));
		tmp.pid = words[0];
		tmp.jetton = words[1];
		tmp.money = words[2];
		svec.push_back(tmp);
		if (words[0] == this->pid)
			this->myseat = tmp;
	}
	return svec;
}

std::vector<Player::Blind> Player::getBlindInfo(){
	typedef std::string::size_type size_type;
	std::string txt = getRecvinfo();
	std::vector<std::string> lines = splitLine(txt);
	std::vector<Blind> bvec;
	for (int i=1;i<=lines.size()-2;++i){
		std::string line = lines[i];
		size_type pos = line.find_first_of(":",0);
		Blind tmp;
		tmp.pid = line.substr(0,pos);
		tmp.bet = line.substr(pos+2,line.size()-pos-3);
		bvec.push_back(tmp); 
	}
	return bvec;
}


std::vector<Player::Holdcard> Player::getHoldCards(){
	std::string txt = getRecvinfo();
	std::vector<std::string> lines = splitLine(txt);
	std::vector<Holdcard> hvec;
	for (int i=1;i<=lines.size()-2;++i){
		std::string line = lines[i];
		std::vector<std::string> words = splitWhiteSpace(line);
		Holdcard tmp;
		tmp.color = words[0];
		tmp.point = words[1];
		hvec.push_back(tmp);
	}
	return hvec;
}

Player::Inquire Player::getInquire(){
	std::string txt = getRecvinfo();
	std::vector<std::string> lines = splitLine(txt);
	Inquire result;
	for (int i=1;i<=lines.size()-3;++i){
		std::string line = lines[i];
		std::vector<std::string> words = splitWhiteSpace(line);
		Status status;
		status.pid = words[0];
		status.jetton = words[1];
		status.money = words[2];
		status.bet = words[3];
		status.action = words[4];
		result.allStatus.push_back(status);
	}
	std::string last = lines[lines.size()-2];
	int pos = last.find_first_of(":",0);
	result.potnum = last.substr(pos+2,last.size()-pos-3);
	return result;
}

int Player::sendAction(const std::string& action){
	std::string ret = std::string(action+" \n");
	std::cout<<"Action: "<<ret<<std::endl;
	const char *tmp = ret.c_str();
	int siz = 0;
	//send register message
	if ((siz = send(sockfd,tmp,strlen(tmp),0)) >= 0)
		return siz;
	else
		return -1;
}

std::vector<Player::Flop> Player::getFlopMsg(){
	std::string txt = getRecvinfo();
	std::vector<Flop> result;
	std::vector<std::string> lines = splitLine(txt);
	for (int i=1;i<=lines.size()-2;++i){
		std::string line = lines[i];
		std::vector<std::string> words = splitWhiteSpace(line);
		Flop tmp;
		tmp.color = words[0];
		tmp.point = words[1];
		result.push_back(tmp);
	}	
	return result;
}

Player::Turn Player::getTurnMsg(){
	std::string txt = getRecvinfo();
	std::vector<std::string> lines = splitLine(txt);
	std::vector<std::string> words = splitWhiteSpace(lines[1]);
	Turn tmp;
	tmp.color = words[0];
	tmp.point = words[1];
	return tmp;
}

Player::River Player::getRiverMsg(){
	std::string txt = getRecvinfo();
	std::vector<std::string> lines = splitLine(txt);
	std::vector<std::string> words = splitWhiteSpace(lines[1]);
	River tmp;
	tmp.color = words[0];
	tmp.point = words[1];
	return tmp;
}

Player::ShowDown Player::getShowDown(){
	std::string txt = getRecvinfo();
	std::vector<std::string> lines = splitLine(txt);
	int row = 2;
	ShowDown result;
	for (int row=2;row<=lines.size()-2;++row){
		std::string line = lines[row];
		if (line.substr(0,7) == "/common"){
			++row;
			break;
		}
		std::vector<std::string> words = splitWhiteSpace(line);
		Common common;
		common.color = words[0];
		common.point = words[1];
		result.commons.push_back(common);
	}
	for (;row<=lines.size()-2;++row){
		std::string line = lines[row];
		std::vector<std::string> words = splitWhiteSpace(line);
		Rank rank;
		rank.pid = words[0];
		rank.card1.color = words[1];
		rank.card1.point = words[2];
		rank.card2.color = words[3];
		rank.card2.point = words[4];
		rank.nut_hand = words[5];
		result.ranks.push_back(rank);
	}
	return result;
}

std::vector<Player::Pot> Player::getPotWin(){
	typedef std::string::size_type size_type;
	std::string txt = getRecvinfo();
	std::vector<std::string> lines = splitLine(txt);
	std::vector<Pot> pvec;
	for (int i=1;i<=lines.size()-2;++i){
		std::string line = lines[i];
		size_type pos = line.find_first_of(":",0);
		Pot tmp;
		tmp.pid = line.substr(0,pos);
		tmp.num = line.substr(pos+2,line.size()-pos-3);
		pvec.push_back(tmp); 
	}
	return pvec;
}

Player::Notify Player::getNotifyMsg(){
	return getInquire();
}

Player::Color Player::getColor(std::string& color){
	if (color == "SPADES")
		return SPADES;
	if (color == "HEARTS")
		return HEARTS;
	if (color == "CLUBS")
		return CLUBS;
	if (color == "DIAMONDS")
		return DIAMONDS;
}


/*
* Utility function
*/
/********************************************************/
int pointtoint(std::string& str){
	if (str.size() == 1){
		char cp = str[0];
		if (cp >= '2' && cp <= '9')
			return strtoint(str);
		else{
			if (cp == 'J')
				return 11;
			if (cp == 'Q')
				return 12;
			if (cp == 'K')
				return 13;
			if (cp == 'A')
				return 14;
		}
	}else
		return 10;//only 10 point occupy two chars	
}
/********************************************************/
void Player::preFlopAction(){
	if (myhc.size() != 2){
		std::cout<<"Hold card's size is not correct"<<std::endl;
		return ;
	}
	double res = 0;
	//my handcard point
	int point1 = pointtoint(myhc[0].point);
	int point2 = pointtoint(myhc[1].point);
	//my handcars color
	Color color1 = getColor(myhc[0].color);
	Color color2 = getColor(myhc[1].color);
	//starting Bill Chen's Formula computing
	int max = std::max(point1,point2);
	int gap = std::abs(point1-point2);
	if (max <= 10)
		res += max/2;//2~10
	else{
		if (max == 14)
			res += 10;//A
		else
			res += max-5;//J,Q,K
	}
	if (point1 == point2){
		//a suite
		res *= 2;
		if (res < 5)
			res = 5;
	}else{
		//not a suite
		if (gap == 2)
			res -= 1;
		if (gap == 3)
			res -= 2;
		if (gap == 4)
			res -= 4;
		if (gap >= 5)
			res -= 5;
	}
	if (color1 == color2)
		res += 2;
	if (point1 < 12 && point2 < 12 && gap < 3)
		res += 1;
	int score = std::ceil(res);
	std::cout<<"preflop score: "<<score<<std::endl;
	sendAction(Action::CHECK);
}

void Player::flopAction(){
	//3 public cards on board
	sendAction(Action::CHECK);
}


void Player::turnAction(){
	//3 public cards and 1 turn card on board
	sendAction(Action::CHECK);
}

void Player::riverAction(){
	//3 public cards and 1 river card on board
	sendAction(Action::CHECK);
}

double Player::cardPower(std::vector<Holdcard> myhc,std::vector<Flop> flops){
	double result = 0.0;
	//my handcard point
	int p1 = pointtoint(myhc[0].point);
	int p2 = pointtoint(myhc[1].point);
	//flop card point
	int p3 = pointtoint(flops[0].point);
	int p4 = pointtoint(flops[1].point);
	int p5 = pointtoint(flops[2].point);
}

#endif
