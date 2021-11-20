#include <unistd.h>     
#include <sys/types.h>  
#include <sys/wait.h>   
#include <iostream>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <fcntl.h> 
#include <dirent.h>
#include <fstream>
#include "DKA.h"

using namespace std;
const vector< char> Sigma {
    'q','w','e','r','t','y','u','i','o','p',
	'a','s','d','f','g','h','j','k','l','z','x',
    'c','v','b','n','m','.','Q','W','E','R','T',
	'Y','U','I','O','P','A','S','D','F','G','H',
	'J','K','L','Z','X','C','V','B','N','M' ,'0',
	'1','2','3','4','5','6','7','8','9','-'
};

//Преобразование выражения типа a*b? в a\Sigma*b\Sigma 
string str_to_regex( const string &str ) {
    string temp;
	//cout << str  << '\n';
    for(int i = 0 ; i < str.size() ; ++i) {
        if(str[i] == '*') {
            temp += "(";
            for(int i = 0 ; i < Sigma.size() - 1 ; ++i)  temp = temp + Sigma[i] + "|";
            temp+=Sigma[Sigma.size()-1];
            temp += ")*";
        } else if(str[i] == '?') {
            temp += "(";
            for(int i = 0 ; i < Sigma.size() - 1 ; ++i)  temp = temp + Sigma[i] + "|";
            temp+=Sigma[Sigma.size()-1];
            temp += ")";
        } else {
            temp += str[i];
        }
    }
	return temp;
}

//Проверка на принадлежность слова path регулярного выражения str
bool word_in_regex (const string & str, const string & path) {
    re_dfa_type dfa;
    bool ok = dfa.compile (str.cbegin (), str.cend ());
    int current = 0;
    for(int i = 0 ; i < path.size() ; ++i) {
        map<int, int> k = dfa.m_transition[current];
        //cout << "[" << current << "-->" <<k[(int)path[i]] << "]" << '\n'; 
		if (k.count((int)path[i])){
			current = k[(int)path[i]];
		} else {
			return 0;
		}
        
    }
    if((dfa.m_transition[current])[-1] == -1) return 1;
    return 0;
}


//Объединение массивов
vector<pair<string,int>> summ_massive(const vector<pair<string,int>> &str1 ,const vector<pair<string,int>> &str2) {
  vector<pair<string,int>> res ;
  for(int i = 0; i < str1.size() ; ++i) {
    res.push_back(str1[i]);
  }
  for(int i = 0; i < str2.size() ; ++i) {
    res.push_back(str2[i]);
  }
  return res ;
}

//Вывести name всех файлов лежащих в path принадлежащие регулярному языку L(regex)
vector<pair<string, int>> path_in_regex(const string& path ,const string& regex ) {
    //if( path == "") path = "/";
    
    vector<pair<string, int>> res;
    DIR *dir;
    struct dirent *de;
    
    dir = opendir( path.c_str()); /*your directory*/
    
	while(dir) {
        //cout << '\n' << path;
        de = readdir(dir);
        if (!de) break;
        //cout << "[" << de->d_name << "]\n";
		//cout << regex << '\n';
		if(de->d_name[0] != '.' && word_in_regex(regex ,de->d_name)) {
          if(path == "/") {;
		    res.push_back({path +(string)de->d_name, 0});
	
		  } else {

            if (de->d_type == 4) 
				res.push_back({path + "/" +(string)de->d_name, 0});
			else
				res.push_back({path + "/" +(string)de->d_name, 1});
	
		  }
        }
    }
    closedir(dir);
    //for(auto k : res ) cout <<"_" << k << "_"; 
	//cout<<endl;
	return res;
}

//Вывести итоговый массив путей принадлежащих языку типа  L(/*a./asd?/asd/asas)
vector<pair<string, int>> regex_to_massive(vector<string>& mass) {
	vector<pair<string, int>> mass_path ;
	if(mass[0] == "/") {
		mass_path.push_back({"/", 0});
	} else  {
		mass_path.push_back({".", 0});
	}
	for(int i = 0 ; i < mass.size() ; ++i) {
		vector<pair<string, int>> mass_path_temp;
		vector<pair<string, int>> temp;
		string regex = str_to_regex(mass[i]);
		if( mass[i] != "/" && mass[i] != "." && mass[i] != "*") {
			for(int j = 0 ; j < mass_path.size() ;++j) {
				temp = path_in_regex(mass_path[j].first , regex);
				mass_path_temp = summ_massive(mass_path_temp , temp);
			}
			mass_path = mass_path_temp;
		}else if (mass[i] == "*"){
			vector<pair<string, int>> temp;
			for(auto i : mass_path){
                DIR *dir;
                struct dirent *de;
                dir = opendir(i.first.c_str());
                while(dir){
                    de = readdir(dir);
                    if (!de) break;
                    if (de->d_name[0] != '.' && de->d_type == 4){
                        temp.push_back({i.first+'/'+(string)de->d_name, 0});
                    }else if (de->d_name[0] != '.' && de->d_type == 8){
						temp.push_back({i.first+'/'+(string)de->d_name, 1});
					}
                }
                closedir(dir);                
            }
			mass_path = temp;
		}
    }
    return mass_path;	
}

//char** to vector<string>
vector<string> reformat_char(char** args) {
	vector<string> ans;
	while (*args != NULL) {
		ans.push_back(*args);
		args++;
	}
	return ans;
}

// /asdas/ to {"/" , "asdas" , "/"}
vector<string> exp_to_massive(string a) {
	vector<string> tokens;
	string temp;
	char c;
	while (!a.empty()) {
		if (a.rfind("/") != a.npos) {
			auto it = a.rfind("/");
			temp = a.substr(it + 1);
			if (!temp.empty()) tokens.push_back(temp);
			tokens.push_back("/");
			a = a.substr(0, it);
		}
		else {
			tokens.push_back(a);
			break;
		}
	}
	reverse(tokens.begin(), tokens.end());
	return tokens;
}

//Запуск функции ls
int exec_ls(char** args) {
	vector<string> argums = reformat_char(args);
	if (argums.size() == 1 || argums.back()[0] == '-') {
			vector<char*> args0; 
			for (size_t i = 0; i < argums.size(); ++i) {
				args0.push_back((char*)argums[i].c_str());
			}
			args0.push_back(nullptr);
			pid_t pid;
			pid = fork();
			if (pid == 0) {
				execvp(args0[0], &args0[0]);
				perror(args0[0]);
				exit(0);
			}
			else if (pid > 0) {
				wait(0);
			}

		return 1;
	}
	else {
		vector<string> folders = exp_to_massive(argums.back());
		vector<pair<string, int>> ans;
		ans = regex_to_massive(folders);

		for(auto k : ans ) 
		if (k.second == 1){
			argums.back() = k.first;
			vector<char* > args0;
				for (size_t i = 0; i < argums.size(); ++i) {
					args0.push_back((char*)argums[i].c_str());
				}
				args0.push_back(nullptr);
				pid_t pid;
				pid = fork();
				if (pid == 0) {
					execvp(args0[0], &args0[0]);
					perror(args0[0]);
					exit(0);
				}
				else if (pid > 0) {
					wait(0);
					continue;
				}
		}
			for (int i = 0; i < ans.size(); i++) {
				argums.back() = ans[i].first;
				if (ans[i].second == 0){
					cout << argums.back() << ':' << endl;	
				}else{
					continue;
				}
				vector<char* > args0;
				for (size_t i = 0; i < argums.size(); ++i) {
					args0.push_back((char*)argums[i].c_str());
				}
				args0.push_back(nullptr);
				pid_t pid;
				pid = fork();
				if (pid == 0) {
					execvp(args0[0], &args0[0]);
					perror(args0[0]);
					exit(0);
				}
				else if (pid > 0) {
					wait(0);
					continue;
				}
			}
	}
	return 1;
}

//Exit
int exec_exit(char** args) {
	exit(0);
	return 0;
}

//"ls -l /home" to {{"ls", "-l" ,"/home"},3}
pair<char**, int> parse_line(char* line) {
	int i = 0;
    string buf; 
    stringstream ss(line);
    vector<char*> tokens;
	while (ss >> buf){
		char * temp = new char[buf.length()+1];
    	strcpy(temp,buf.c_str());
    	tokens.push_back(temp);
	}
		
    char** argv = new char*[tokens.size() + 1];
	
    for ( int k = 0; k < tokens.size(); k++ ){
      	argv[k] = tokens[k];
		//cout << "[" << tokens[k] << "]" ;
	}
	//cout << tokens.size();
	argv[tokens.size()] = nullptr;
	return {argv, tokens.size()};
}

// pair<char**, int> parse_line(char* line) {
//     //string copy = (string)line;
//     char line_copy[strlen(line)+1];
//     strcpy(line_copy,line);
//     //printf("%s" , line_copy);

//     vector<char*> tokens;
//    char sep [10]= " ";
//    char *istr;
//    istr = strtok (line_copy,sep);
//    while (istr != NULL) {
//      tokens.push_back(istr);
//      // printf (“%s\n”,istr);
//       istr = strtok (NULL,sep);
//    }
//     char** argv = new char*[tokens.size() + 1];
//     for ( int k = 0; k < tokens.size(); k++ ){
//       	argv[k] = tokens[k];
//         cout << "[" << tokens[k] << "]";
//     }
// 	cout << tokens.size();
// 	argv[tokens.size()] = nullptr;
// 	return {argv, tokens.size()};
// }

void Execution(char *cmd,char* args[], int argnum, int& Stop) {
int pipes = 0;
    for(int i = 0; i < argnum; i++){
        if(strcmp(args[i], "|") == 0) {pipes = 1; }
    }
	//Реализация труб( последовательно исполняем процессы и передаем слева направо)
	if(pipes) {
        for(int i = 0; i < argnum; i++){
		if(strcmp(args[i], "|") == 0){
			args[i] = NULL;
			char* right[argnum - i];
			int c = 0;
			for(int j = i; j < argnum - 1; j++){
					right[j - i] = args[j + 1];
					c++;
			}
			right[c] = NULL;
			int p[2];
			pipe(p);
			pid_t lpid = fork();
			if(lpid == 0) {//left (child)
				dup2(2, 1);
				dup2(p[1], STDOUT_FILENO);
				execvp(cmd, args);
			}
			else {//right (parent)
				pid_t rpid = fork();
				close(p[1]);
				if(rpid == 0) { //right child
					dup2(2, 1);
					dup2(p[0], STDIN_FILENO);
					Execution(right[0], right, c, Stop);
					Stop = 1;
					close(p[1]);
				} else { //parent
					waitpid(rpid, 0, 0);
				}
			}
			break;
			}		
        }
    }
    else {
		// Обработка отдельно комманд ls and cd 
        if(strcmp(cmd, "cd") == 0){ //built-in command cd
            if(args[1] == NULL){
                fprintf(stderr, "lsh: expected argument to \"cd\"\n");
            }
            else{
                chdir(args[1]);
            }
        }
		else 
		if(strcmp(cmd, "ls") == 0) { //ls
            exec_ls(args);
        }
        else{
            pid_t pid = fork();
            if( pid < 0){
                perror("Error when forked");  
				Stop = 1;               
            }
            else 
			if(pid == 0){ //процесс ребенка
				for(int i = 0; i < argnum; i++){
					//Направление стандартного ввода вывода
					if(strcmp(args[i], ">") == 0){
						int newstdout = open(args[i + 1], O_WRONLY | O_CREAT , S_IRWXU | S_IRWXG | S_IRWXO);
						close(1);
						dup(newstdout);
						close(newstdout);
						args[i] = NULL;
					}
					else if(strcmp(args[i], "<") == 0){
						int newstdin = open(args[i + 1], O_RDONLY);
						close(0);
						dup(newstdin);
						close(newstdin);
						args[i] = NULL;
					}
				}
				execvp(cmd, args);
				perror(cmd);
				Stop = 1;
            }
			else{
				if (waitpid(pid, 0, 0) < 0) { //родительский процесс
          			Stop = 1;
	  				perror("Error when waiting for child");
        		}
      	  	}
            
        }
    }
}

//Считывание , парсинг,исполнение ,завершение
void micro_start() {
	char* line;
	char** args;
	int Count;
	string Line;
	int Stop = 0;
	while(!Stop) {
		int p = 0;
		pid_t uid = getuid();
		if (p == 0) {
			char str[100];
			getcwd(str, 100 * sizeof(char));
			if (uid == 0) {
				cout<<"[" << str<<"] !";
			}
			else {
				cout<<"[" << str<<"] >";
			}
		}

		if(!getline(cin,Line) ){ Stop = 1; }
		char *line = (char*)Line.c_str();
		if (Line.size() == 0) {
			continue;
		}
		pair<char**, int> Tokens = parse_line(line);
		args = Tokens.first;
		Count = Tokens.second;
		Execution(args[0],args, Count, Stop);
		delete[]args;
	}
}
int main(int argc, char** argv) {
	//Запуск цикла
	micro_start();
	return 0;
}