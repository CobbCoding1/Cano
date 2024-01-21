#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexerf.h"

#define MAX_CURLY_STACK_LENGTH 64

typedef struct Node {
  char *value;
  TokenType type;
  struct Node *right;
  struct Node *left;
} Node;

typedef struct {
  Node *content[MAX_CURLY_STACK_LENGTH];
  int top;
} curly_stack;

Node *peek_curly(curly_stack *stack){
  return stack->content[stack->top];
}

void push_curly(curly_stack *stack, Node *element){
  stack->top++;
  stack->content[stack->top] = element;
}

Node *pop_curly(curly_stack *stack){
  Node *result = stack->content[stack->top];
  stack->top--;
  return result;
}

void print_tree(Node *node, int indent, char *identifier){
  if(node == NULL){
    return;
  }
  for(int i = 0; i < indent; i++){
    printf(" ");
  }
  printf("%s -> ", identifier);
  for(size_t i = 0; node->value[i] != '\0'; i++){
    printf("%c", node->value[i]);
  }
  printf("\n");
  print_tree(node->left, indent + 1, "left");
  print_tree(node->right, indent + 1, "right");
}

Node *init_node(Node *node, char *value, TokenType type){
  node = malloc(sizeof(Node));
  node->value = malloc(sizeof(char) * 2);
  node->type = (int)type;
  node->value = value;
  node->left = NULL;
  node->right = NULL;
  return node;
}

void print_error(char *error_type, size_t line_number){
  printf("ERROR: %s on line number: %zu\n", error_type, line_number);
  exit(1);
}

Node *parse_expression(Token *current_token, Node *current_node){
  Node *expr_node = malloc(sizeof(Node));
  expr_node = init_node(expr_node, current_token->value, current_token->type);
  current_token++;
  if(current_token->type != OPERATOR){
    return expr_node;
  }
  return expr_node;
}


Token *generate_operation_nodes(Token *current_token, Node *current_node){
  Node *oper_node = malloc(sizeof(Node));
  oper_node = init_node(oper_node, current_token->value, OPERATOR);
  current_node->left->left = oper_node;
  current_node = oper_node;
  current_token--;
  if(current_token->type == INT){
    Node *expr_node = malloc(sizeof(Node));
    expr_node = init_node(expr_node, current_token->value, INT);
    current_node->left = expr_node;
  } else if(current_token->type == IDENTIFIER){
    Node *identifier_node = malloc(sizeof(Node));
    identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
    current_node->left = identifier_node;
  } else {
    printf("ERROR: expected int or identifier\n");
    exit(1);
  }
  current_token++;
  current_token++;
  while(current_token->type == INT || current_token->type == IDENTIFIER || current_token->type == OPERATOR){
    if(current_token->type == INT || current_token->type == IDENTIFIER){
      if((current_token->type != INT && current_token->type != IDENTIFIER) || current_token == NULL){
        printf("Syntax Error hERE\n");
        exit(1);
      }
      current_token++;
      if(current_token->type != OPERATOR){
        current_token--;
        if(current_token->type == INT){
          Node *second_expr_node = malloc(sizeof(Node));
          second_expr_node = init_node(second_expr_node, current_token->value, INT);
          current_node->right = second_expr_node;
        } else if(current_token->type == IDENTIFIER){
          Node *second_identifier_node = malloc(sizeof(Node));
          second_identifier_node = init_node(second_identifier_node, current_token->value, IDENTIFIER);
          current_node->right = second_identifier_node;
        } else {
          printf("ERROR: Expected Integer or Identifier\n");
          exit(1);
        }
      }
    }
    if(current_token->type == OPERATOR){
      Node *next_oper_node = malloc(sizeof(Node));
      next_oper_node = init_node(next_oper_node, current_token->value, OPERATOR);
      current_node->right = next_oper_node;
      current_node = next_oper_node;
      current_token--;
      if(current_token->type == INT){
        Node *second_expr_node = malloc(sizeof(Node));
        second_expr_node = init_node(second_expr_node, current_token->value, INT);
        current_node->left = second_expr_node;
      } else if(current_token->type == IDENTIFIER){
        Node *second_identifier_node = malloc(sizeof(Node));
        second_identifier_node = init_node(second_identifier_node, current_token->value, IDENTIFIER);
        current_node->left = second_identifier_node;
      } else {
        printf("ERROR: Expected IDENTIFIER or INT\n");
        exit(1);
      }
      current_token++; 
    }
    current_token++;
  }
  return current_token;
}

Node *handle_exit_syscall(Node *root, Token *current_token, Node *current){
    Node *exit_node = malloc(sizeof(Node));
    exit_node = init_node(exit_node, current_token->value, KEYWORD);
    current->right = exit_node;
    current = exit_node;
    current_token++;
    if(current_token->type == END_OF_TOKENS){
      print_error("Invalid Syntax on OPEN", current_token->line_num);
    }
    if(strcmp(current_token->value, "(") == 0 && current_token->type == SEPARATOR){
      Node *open_paren_node = malloc(sizeof(Node));
      open_paren_node = init_node(open_paren_node, current_token->value, SEPARATOR);
      current->left = open_paren_node;
      current_token++;
      if(current_token->type == END_OF_TOKENS){
        print_error("Invalid Syntax on INT", current_token->line_num);
      }
      if(current_token->type == INT || current_token->type == IDENTIFIER){
        current_token++;
        if(current_token->type == OPERATOR && current_token != NULL){
          current_token = generate_operation_nodes(current_token, current);
          current_token--;
        } else {
          current_token--;
          Node *expr_node = malloc(sizeof(Node));
          expr_node = init_node(expr_node, current_token->value, current_token->type);
          current->left->left = expr_node;
        }
        current_token++;
        if(current_token->type == END_OF_TOKENS){
          print_error("Invalid Syntax on cLOSE", current_token->line_num);
        }
        if(strcmp(current_token->value, ")") == 0 && current_token->type == SEPARATOR && current_token->type != END_OF_TOKENS){
          Node *close_paren_node = malloc(sizeof(Node));
          close_paren_node = init_node(close_paren_node, current_token->value, SEPARATOR);
          current->left->right = close_paren_node;
          current_token++;
          if(current_token->type == END_OF_TOKENS){
            print_error("Invalid Syntax on SEMI", current_token->line_num);
          }
          if(strcmp(current_token->value, ";") == 0 && current_token->type == SEPARATOR){
            Node *semi_node = malloc(sizeof(Node));
            semi_node = init_node(semi_node, current_token->value, SEPARATOR);
            current->right = semi_node;
            current = semi_node;
          } else {
            print_error("Invalid Syntax on SEMI", current_token->line_num);
          }
        } else {
            print_error("Invalid Syntax on CLOSE", current_token->line_num);
        }
      } else {
        print_error("Invalid Syntax INT", current_token->line_num);
      }
  } else {
    print_error("Invalid Syntax OPEN", current_token->line_num);
  }
  return current;
}

void handle_token_errors(char *error_text, Token *current_token, TokenType type){
  if(current_token->type == END_OF_TOKENS || current_token->type != type){
    print_error(error_text, current_token->line_num);
  }
}

Node *create_variable_reusage(Token *current_token, Node *current){
  Node *main_identifier_node = malloc(sizeof(Node));
  main_identifier_node = init_node(main_identifier_node, current_token->value, IDENTIFIER);
  current->left = main_identifier_node;
  current = main_identifier_node;
  current_token++;

  handle_token_errors("Invalid syntax after idenfitier", current_token, OPERATOR);

  if(current_token->type == OPERATOR){
    if(strcmp(current_token->value, "=") != 0){
      print_error("Invalid Variable Syntax on =", current_token->line_num);
    }
    Node *equals_node = malloc(sizeof(Node));
    equals_node = init_node(equals_node, current_token->value, OPERATOR);
    current->left = equals_node;
    current = equals_node;
    current_token++;
  }
  if(current_token->type == END_OF_TOKENS || (current_token->type != INT && current_token->type != IDENTIFIER)){
    print_error("Invalid Syntax After Equals", current_token->line_num);
  }

  current_token++;
  if(current_token->type == OPERATOR){
    Node *oper_node = malloc(sizeof(Node));
    oper_node = init_node(oper_node, current_token->value, OPERATOR);
    current->left = oper_node;
    current = oper_node;
    current_token--;
    if(current_token->type == INT){
      Node *expr_node = malloc(sizeof(Node));
      expr_node = init_node(expr_node, current_token->value, INT);
      oper_node->left = expr_node;
      current_token++;
      current_token++;
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = malloc(sizeof(Node));
      identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
      oper_node->left = identifier_node;
      current_token++;
      current_token++;
    } else {
      print_error("ERROR: Expected IDENTIFIER or INT", current_token->line_num);
    }
    current_token++;

    if(current_token->type == OPERATOR){
      Node *oper_node = malloc(sizeof(Node));
      oper_node = init_node(oper_node, current_token->value, OPERATOR);
      current->right = oper_node;
      current = oper_node;
      int operation = 1;
      current_token--;
      current_token--;
      while(operation){
        current_token++;
        if(current_token->type == INT){
          Node *expr_node = malloc(sizeof(Node));
          expr_node = init_node(expr_node, current_token->value, INT);
          current->left = expr_node;
        } else if(current_token->type == IDENTIFIER){
          Node *identifier_node = malloc(sizeof(Node));
          identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
          current->left = identifier_node;
        } else {
          printf("ERROR: Unexpected Token\n", current_token->line_num);
          exit(1);
        }
        current_token++;
        if(current_token->type == OPERATOR){
          current_token++;
          current_token++;
          if(current_token->type != OPERATOR){
            current_token--;
            if(current_token->type == INT){
              Node *expr_node = malloc(sizeof(Node));
              expr_node = init_node(expr_node, current_token->value, INT);
              current->right = expr_node;
              current_token++;
            } else if(current_token->type == IDENTIFIER){
              Node *identifier_node = malloc(sizeof(Node));
              identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
              current->right = identifier_node;
              current_token++;
            } else {
              printf("ERROR: UNRECOGNIZED TOKEN!\n");
              exit(1);
            }
            operation = 0;
          } else {
            current_token--;
            current_token--;
            Node *oper_node = malloc(sizeof(Node));
            oper_node = init_node(oper_node, current_token->value, OPERATOR);
            current->right = oper_node;
            current = oper_node;
          }
        } else {
          operation = 0;
        }
      }
    } else {
      current_token--;
      if(current_token->type == INT){
        Node *expr_node = malloc(sizeof(Node));
        expr_node = init_node(expr_node, current_token->value, INT);
        oper_node->right = expr_node;
      } else if(current_token->type == IDENTIFIER){
        Node *identifier_node = malloc(sizeof(Node));
        identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
        oper_node->right = identifier_node;
      }
      current_token++;
    }
  } else {
    current_token--;
    if(current_token->type == INT){
      Node *expr_node = malloc(sizeof(Node));
      expr_node = init_node(expr_node, current_token->value, INT);
      current->left = expr_node;
      current_token++;
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = malloc(sizeof(Node));
      identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
      current->left = identifier_node;
      current_token++;
    }
  }
  handle_token_errors("Invalid Syntax After Expression", current_token, SEPARATOR);

  current = main_identifier_node;
  if(strcmp(current_token->value, ";") == 0){
    Node *semi_node = malloc(sizeof(Node));
    semi_node = init_node(semi_node, current_token->value, SEPARATOR);
    current->right = semi_node;
    current = semi_node;
  }
  return current;
}


Node *create_variables(Token *current_token, Node *current){
  Node *var_node = malloc(sizeof(Node));
  var_node = init_node(var_node, current_token->value, KEYWORD);
  current->left = var_node;
  current = var_node;
  current_token++;
  handle_token_errors("Invalid syntax after INT", current_token, IDENTIFIER);
  if(current_token->type == IDENTIFIER){
    Node *identifier_node = malloc(sizeof(Node));
    identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
    current->left = identifier_node;
    current = identifier_node;
    current_token++;
  }
  handle_token_errors("Invalid Syntax After Identifier", current_token, OPERATOR);

  if(current_token->type == OPERATOR){
    if(strcmp(current_token->value, "=") != 0){
      print_error("Invalid Variable Syntax on =", current_token->line_num);
    }
    Node *equals_node = malloc(sizeof(Node));
    equals_node = init_node(equals_node, current_token->value, OPERATOR);
    current->left = equals_node;
    current = equals_node;
    current_token++;
  }
  if(current_token->type == END_OF_TOKENS || (current_token->type != INT && current_token->type != IDENTIFIER)){
    print_error("Invalid Syntax After Equals", current_token->line_num);
  }

  current_token++;
  if(current_token->type == OPERATOR){
    Node *oper_node = malloc(sizeof(Node));
    oper_node = init_node(oper_node, current_token->value, OPERATOR);
    current->left = oper_node;
    current = oper_node;
    current_token--;
    if(current_token->type == INT){
      Node *expr_node = malloc(sizeof(Node));
      expr_node = init_node(expr_node, current_token->value, INT);
      oper_node->left = expr_node;
      current_token++;
      current_token++;
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = malloc(sizeof(Node));
      identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
      oper_node->left = identifier_node;
      current_token++;
      current_token++;
    } else {
      print_error("ERROR: Expected IDENTIFIER or INT", current_token->line_num);
    }
    current_token++;

    if(current_token->type == OPERATOR){
      Node *oper_node = malloc(sizeof(Node));
      oper_node = init_node(oper_node, current_token->value, OPERATOR);
      current->right = oper_node;
      current = oper_node;
      int operation = 1;
      current_token--;
      current_token--;
      while(operation){
        current_token++;
        if(current_token->type == INT){
          Node *expr_node = malloc(sizeof(Node));
          expr_node = init_node(expr_node, current_token->value, INT);
          current->left = expr_node;
        } else if(current_token->type == IDENTIFIER){
          Node *identifier_node = malloc(sizeof(Node));
          identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
          current->left = identifier_node;
        } else {
          printf("ERROR: Unexpected Token\n");
          exit(1);
        }
        current_token++;
        if(current_token->type == OPERATOR){
          current_token++;
          current_token++;
          if(current_token->type != OPERATOR){
            current_token--;
            if(current_token->type == INT){
              Node *expr_node = malloc(sizeof(Node));
              expr_node = init_node(expr_node, current_token->value, INT);
              current->right = expr_node;
              current_token++;
            } else if(current_token->type == IDENTIFIER){
              Node *identifier_node = malloc(sizeof(Node));
              identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
              current->right = identifier_node;
              current_token++;
            } else {
              printf("ERROR: UNRECOGNIZED TOKEN!\n");
              exit(1);
            }
            operation = 0;
          } else {
            current_token--;
            current_token--;
            Node *oper_node = malloc(sizeof(Node));
            oper_node = init_node(oper_node, current_token->value, OPERATOR);
            current->right = oper_node;
            current = oper_node;
          }
        } else {
          operation = 0;
        }
      }
    } else {
      current_token--;
      if(current_token->type == INT){
        Node *expr_node = malloc(sizeof(Node));
        expr_node = init_node(expr_node, current_token->value, INT);
        oper_node->right = expr_node;
      } else if(current_token->type == IDENTIFIER){
        Node *identifier_node = malloc(sizeof(Node));
        identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
        oper_node->right = identifier_node;
      }
      current_token++;
    }
  } else {
    current_token--;
    if(current_token->type == INT){
      Node *expr_node = malloc(sizeof(Node));
      expr_node = init_node(expr_node, current_token->value, INT);
      current->left = expr_node;
      current_token++;
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = malloc(sizeof(Node));
      identifier_node = init_node(identifier_node, current_token->value, IDENTIFIER);
      current->left = identifier_node;
      current_token++;
    }
  }

  //if(current_token->type == OPERATOR){
  //  current_token = generate_operation_nodes(current_token, current);
  //}

  handle_token_errors("Invalid Syntax After Expression", current_token, SEPARATOR);

  current = var_node;
  if(strcmp(current_token->value, ";") == 0){
    Node *semi_node = malloc(sizeof(Node));
    semi_node = init_node(semi_node, current_token->value, SEPARATOR);
    current->right = semi_node;
    current = semi_node;
  }
  return current;
}

Token *generate_if_operation_nodes(Token *current_token, Node *current_node){
  Node *oper_node = malloc(sizeof(Node));
  oper_node = init_node(oper_node, current_token->value, OPERATOR);
  current_node->left->left = oper_node;
  current_node = oper_node;
  current_token--;

  Node *expr_node = malloc(sizeof(Node));
  expr_node = init_node(expr_node, current_token->value, current_token->type);
  current_node->left = expr_node;

  current_token++;
  current_token++;
  while(current_token->type == INT || current_token->type == IDENTIFIER || current_token->type == OPERATOR){
    if(current_token->type == INT || current_token->type == IDENTIFIER){
      if((current_token->type != INT && current_token->type != IDENTIFIER) || current_token == NULL){
        exit(1);
      }
      current_token++;
      if(current_token->type != OPERATOR || strcmp(current_token->value, "=") == 0){
        current_token--;
        if(current_token->type == INT){
          Node *second_expr_node = malloc(sizeof(Node));
          second_expr_node = init_node(second_expr_node, current_token->value, INT);
          current_node->right = second_expr_node;
        } else if(current_token->type == IDENTIFIER){
          Node *second_identifier_node = malloc(sizeof(Node));
          second_identifier_node = init_node(second_identifier_node, current_token->value, IDENTIFIER);
          current_node->right = second_identifier_node;
        } else {
          printf("ERROR: Expected Integer or Identifier\n");
          exit(1);
        }
      }
    }
    if(strcmp(current_token->value, "=") == 0){
      break;
    } else if(current_token->type == OPERATOR){
      Node *next_oper_node = malloc(sizeof(Node));
      next_oper_node = init_node(next_oper_node, current_token->value, OPERATOR);
      current_node->right = next_oper_node;
      current_node = next_oper_node;
      current_token--;
      if(current_token->type == INT){
        Node *second_expr_node = malloc(sizeof(Node));
        second_expr_node = init_node(second_expr_node, current_token->value, INT);
        current_node->left = second_expr_node;
      } else if(current_token->type == IDENTIFIER){
        Node *second_identifier_node = malloc(sizeof(Node));
        second_identifier_node = init_node(second_identifier_node, current_token->value, IDENTIFIER);
        current_node->left = second_identifier_node;
      } else {
        printf("ERROR: Expected IDENTIFIER or INT\n");
        exit(1);
      }
      current_token++; 
    }
    current_token++;
  }
  return current_token;
}

Token *generate_if_operation_nodes_right(Token *current_token, Node *current_node){
  Node *oper_node = malloc(sizeof(Node));
  oper_node = init_node(oper_node, current_token->value, OPERATOR);
  current_node->left->right = oper_node;
  current_node = oper_node;
  current_token--;

  Node *expr_node = malloc(sizeof(Node));
  expr_node = init_node(expr_node, current_token->value, current_token->type);
  current_node->left = expr_node;

  current_token++;
  current_token++;
  while(current_token->type == INT || current_token->type == IDENTIFIER || current_token->type == OPERATOR){
    if(current_token->type == INT || current_token->type == IDENTIFIER){
      if((current_token->type != INT && current_token->type != IDENTIFIER) || current_token == NULL){
        printf("Syntax Error hERE\n");
        exit(1);
      }
      current_token++;
      if(current_token->type != OPERATOR || strcmp(current_token->value, "=") == 0){
        current_token--;
        if(current_token->type == INT){
          Node *second_expr_node = malloc(sizeof(Node));
          second_expr_node = init_node(second_expr_node, current_token->value, INT);
          current_node->right = second_expr_node;
        } else if(current_token->type == IDENTIFIER){
          Node *second_identifier_node = malloc(sizeof(Node));
          second_identifier_node = init_node(second_identifier_node, current_token->value, IDENTIFIER);
          current_node->right = second_identifier_node;
        } else {
          printf("ERROR: Expected Integer or Identifier\n");
          exit(1);
        }
      }
    }
    if(strcmp(current_token->value, "=") == 0){
      break;
    } else if(current_token->type == OPERATOR){
      Node *next_oper_node = malloc(sizeof(Node));
      next_oper_node = init_node(next_oper_node, current_token->value, OPERATOR);
      current_node->right = next_oper_node;
      current_node = next_oper_node;
      current_token--;
      if(current_token->type == INT){
        Node *second_expr_node = malloc(sizeof(Node));
        second_expr_node = init_node(second_expr_node, current_token->value, INT);
        current_node->left = second_expr_node;
      } else if(current_token->type == IDENTIFIER){
        Node *second_identifier_node = malloc(sizeof(Node));
        second_identifier_node = init_node(second_identifier_node, current_token->value, IDENTIFIER);
        current_node->left = second_identifier_node;
      } else {
        printf("ERROR: Expected IDENTIFIER or INT\n");
        exit(1);
      }
      current_token++; 
    }
    current_token++;
  }
  return current_token;
}

Node *create_if_statement(Token *current_token, Node *current){
  Node *if_node = malloc(sizeof(Node));
  if_node = init_node(if_node, current_token->value, current_token->type);
  current->left = if_node;
  current = if_node;
  current_token++;

  handle_token_errors("ERROR: Expected (", current_token, SEPARATOR);

  Node *open_paren_node = malloc(sizeof(Node));
  open_paren_node = init_node(open_paren_node, current_token->value, SEPARATOR);
  current->left = open_paren_node;
  current = open_paren_node;

  current_token++;

  if(current_token->type != IDENTIFIER && current_token->type != INT){
    printf("ERROR: Expected Identifier or INT\n");
    exit(1);
  }
  

  while(current_token->type != END_OF_TOKENS && current_token->type != COMP){
    current_token++;
  }

  if(current_token->type != COMP){
    printf("ERROR: Expected =\n");
    exit(1);
  }
  Node *comp_node = malloc(sizeof(Node));
  comp_node = init_node(comp_node, current_token->value, current_token->type);
  open_paren_node->left = comp_node;

  while(current_token->type != SEPARATOR){
    current_token--;
  }

  current_token++;
  current_token++;

  if(current_token->type != OPERATOR || current_token->type == COMP){
    current_token--;
    Node *expr_node = malloc(sizeof(Node));
    expr_node = init_node(expr_node, current_token->value, current_token->type);
    comp_node->left = expr_node;
  } else {
    current_token = generate_if_operation_nodes(current_token, current);
  }


  current_token++;
  while((current_token->type != END_OF_TOKENS && current_token->type != OPERATOR && current_token->type != SEPARATOR) || strcmp(current_token->value, "=") == 0){
    current_token++;
  }
  if(current_token->type == SEPARATOR){
    current_token--;
    Node *expr_node = malloc(sizeof(Node));
    expr_node = init_node(expr_node, current_token->value, current_token->type);
    comp_node->right = expr_node;
  } else {
    current_token = generate_if_operation_nodes_right(current_token, current);
  }

  

  Node *close_paren_node = malloc(sizeof(Node));
  close_paren_node = init_node(close_paren_node, ")", SEPARATOR);
  open_paren_node->right = close_paren_node;
  current = close_paren_node;

  return current;
}

Node *handle_write_node(Token *current_token, Node *current){
  Node *write_node = NULL;
  write_node = init_node(write_node, current_token->value, current_token->type);
  current->left = write_node;
  current = write_node;
  
  current_token++;

  handle_token_errors("ERROR: Expected (", current_token, SEPARATOR);

  current_token++;
  if(current_token->type != STRING && current_token->type != IDENTIFIER){
    handle_token_errors("ERROR: Expected String Literal", current_token, STRING);
  }

  Node *string_node = NULL;
  string_node = init_node(string_node, current_token->value, current_token->type);
  current->left = string_node;

  current_token++;

  handle_token_errors("ERROR: Expected ,", current_token, SEPARATOR);

  current_token++;

  Node *number_node = NULL;
  number_node = init_node(number_node, current_token->value, current_token->type);
  current->right = number_node;

  current_token++;

  handle_token_errors("ERROR: Expected )", current_token, SEPARATOR);

  current_token++;

  if(strcmp(current_token->value, ";") != 0){
    print_error("ERROR: Expected ;", current_token->line_num);
  }


  Node *semi_node = NULL;
  semi_node = init_node(semi_node, current_token->value, current_token->type);
  number_node->right = semi_node; 
  current = semi_node;
  return current;
}

Node *parser(Token *tokens){
  Token *current_token = &tokens[0];
  Node *root = malloc(sizeof(Node));
  root = init_node(root, "PROGRAM", BEGINNING);

  Node *current = root;

  Node *open_curly = malloc(sizeof(Node));
  //Node *close_curly = malloc(sizeof(Node));

  curly_stack *stack = malloc(sizeof(curly_stack));

  while(current_token->type != END_OF_TOKENS){
    if(current == NULL){
      break;
    }
    switch(current_token->type){
      case KEYWORD:
        if(strcmp(current_token->value, "EXIT") == 0){
          current = handle_exit_syscall(root, current_token, current);
        } else if(strcmp(current_token->value, "INT") == 0){
          current = create_variables(current_token, current);
        } else if(strcmp(current_token->value, "IF") == 0){
          current = create_if_statement(current_token, current);
        } else if(strcmp(current_token->value, "WHILE") == 0){
          current = create_if_statement(current_token, current);
        } else if(strcmp(current_token->value, "WRITE") == 0){
          current = handle_write_node(current_token, current);
        }
        break;
      case SEPARATOR:
        if(strcmp(current_token->value, "{") == 0){
          Token *temp = current_token;
          open_curly = init_node(open_curly, temp->value, SEPARATOR);
          current->left = open_curly;
          current = open_curly;
          push_curly(stack, open_curly);
          current = peek_curly(stack);
        }
        if(strcmp(current_token->value, "}") == 0){
          Node *close_curly = malloc(sizeof(Node));
          open_curly = pop_curly(stack);
          if(open_curly == NULL){
            printf("ERROR: Expected Open Parenthesis!\n");
            exit(1);
          }
          close_curly = init_node(close_curly, current_token->value, current_token->type);
          current->right = close_curly;
          current = close_curly;
        }
        break; 
      case OPERATOR:
        break;
      case INT:
        break;
      case IDENTIFIER:
        current_token--;
        if(current_token->type == SEPARATOR && ((strcmp(current_token->value, ";") == 0) || (strcmp(current_token->value, "}") == 0) || (strcmp(current_token->value, "{") == 0))){
          current_token++;
          current = create_variable_reusage(current_token, current);
        } else {
          current_token++;
        }
        break;
      case STRING:
        break;
      case COMP:
        break;
      case BEGINNING:
        break;
      case END_OF_TOKENS:
        break;
    }
    current_token++;
  }
  return root;
}
