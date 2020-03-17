#pragma once

#include "AST.h"
#include "parser/NorbertBaseVisitor.h"
#include <utility>

class ASTGen : NorbertBaseVisitor {
public:
    File gen(NorbertParser::FileContext *ctx) {
        return File{visit(ctx)};
    }

private:
    virtual antlrcpp::Any visitFile(NorbertParser::FileContext *ctx) override {
        map<string, Function> l;
        for (auto s : ctx->function()) 
            l.insert(visit(s).as<pair<string, Function>>());
        return l;
    }

    virtual antlrcpp::Any visitFunction(NorbertParser::FunctionContext *ctx) override {
        vector<string> args;
        for (int i=1;i<ctx->ID().size();i++) args.push_back(ctx->ID(i)->getText());
        return make_pair<string, Function>(
            ctx->ID(0)->getText(),
            Function{
                args,
                (ctx->stat())?visit(ctx->stat()).as<statp>():nullptr,
                (ctx->exp())?visit(ctx->exp()).as<expp>():nullptr,
            }
        );
    }

    virtual antlrcpp::Any visitAssignstat(NorbertParser::AssignstatContext *ctx) override {
        return statp(new AssignStat(
            visit(ctx->lexp()),
            visit(ctx->exp())
        ));
    }

    virtual antlrcpp::Any visitFunccallstat(NorbertParser::FunccallstatContext *ctx) override {
        return statp(new FuncCallStat(
            visit(ctx->exp()), visit(ctx->explist())
        ));
    }

    virtual antlrcpp::Any visitWhilestat(NorbertParser::WhilestatContext *ctx) override {
        return statp(new WhileStat(
            visit(ctx->exp()),
            visit(ctx->stat())));
    }

    statp visitIfAux(NorbertParser::IfstatContext *ctx, int index) {
        expp e = visit(ctx->exp(index));
        statp s = visit(ctx->stat(index));
        if (index == ctx->exp().size()) {
            statp els = visit(ctx->els);
            return statp(new IfStat(e,s,els));
        } else {
            return statp(new IfStat(e,s,visitIfAux(ctx, index+1)));
        }
    }

    virtual antlrcpp::Any visitIfstat(NorbertParser::IfstatContext *ctx) override {
        return visitIfAux(ctx, 0);
    }

    virtual antlrcpp::Any visitBlockstat(NorbertParser::BlockstatContext *ctx) override {
        vector<statp> l;
        for (auto s : ctx->stat()) l.push_back(visit(s));
        return statp(new BlockStat(l));
    }

    virtual antlrcpp::Any visitReturnstat(NorbertParser::ReturnstatContext *ctx) override {
        return statp(new ReturnStat(visit(ctx->exp()).as<expp>()));
    }

    virtual antlrcpp::Any visitLexp(NorbertParser::LexpContext *ctx) override {
        if (ctx->ID()) {
            return lexpp(new LexpId(ctx->ID()->getText()));
        } else {
            lexpp p = visit(ctx->lexp());
            expp e = visit(ctx->exp());
            return lexpp(new LexpIndex(p, e));
        }
    }

    virtual antlrcpp::Any visitFunccallexp(NorbertParser::FunccallexpContext *ctx) override {
        return expp(new FuncCallExp(visit(ctx->exp()), visit(ctx->explist())));
    }

    virtual antlrcpp::Any visitAndexp(NorbertParser::AndexpContext *ctx) override {
        return expp(new FuncCallExp(
            expp(new IdExp(ctx->op->getText())),
            {visit(ctx->exp(0)), visit(ctx->exp(1))}
        ));
    }

    virtual antlrcpp::Any visitOrexp(NorbertParser::OrexpContext *ctx) override {
        return expp(new FuncCallExp(
            expp(new IdExp(ctx->op->getText())),
            {visit(ctx->exp(0)), visit(ctx->exp(1))}
        ));
    }

    virtual antlrcpp::Any visitAdditiveexp(NorbertParser::AdditiveexpContext *ctx) override {
        return expp(new FuncCallExp(
            expp(new IdExp(ctx->op->getText())),
            {visit(ctx->exp(0)), visit(ctx->exp(1))}
        ));
    }

    virtual antlrcpp::Any visitFloatexp(NorbertParser::FloatexpContext *ctx) override {
        stringstream ss;
        ss << ctx->FLOAT()->getText();
        float val;
        ss >> val;
        return expp(new FloatExp(val));
    }

    virtual antlrcpp::Any visitRelationexp(NorbertParser::RelationexpContext *ctx) override {
        return expp(new FuncCallExp(
            expp(new IdExp(ctx->op->getText())),
            {visit(ctx->exp(0)), visit(ctx->exp(1))}
        ));
    }

    virtual antlrcpp::Any visitUnaryexp(NorbertParser::UnaryexpContext *ctx) override {
        return expp(new FuncCallExp(
            expp(new IdExp(ctx->op->getText())),
            {visit(ctx->exp()).as<expp>()}
        ));
    }

    virtual antlrcpp::Any visitMultiplicativeexp(NorbertParser::MultiplicativeexpContext *ctx) override {
        return expp(new FuncCallExp(
            expp(new IdExp(ctx->op->getText())),
            {visit(ctx->exp(0)), visit(ctx->exp(1))}
        ));
    }

    virtual antlrcpp::Any visitTrueexp(NorbertParser::TrueexpContext *ctx) override {
        return expp(new IntExp(1));
    }

    virtual antlrcpp::Any visitIdexp(NorbertParser::IdexpContext *ctx) override {
        return expp(new IdExp(ctx->ID()->getText()));
    }

    virtual antlrcpp::Any visitComparisonexp(NorbertParser::ComparisonexpContext *ctx) override {
        return expp(new FuncCallExp(
            expp(new IdExp(ctx->op->getText())),
            {visit(ctx->exp(0)), visit(ctx->exp(1))}
        ));
    }

    virtual antlrcpp::Any visitFalseexp(NorbertParser::FalseexpContext *ctx) override {
        return expp(new IntExp(0));
    }

    virtual antlrcpp::Any visitStringexp(NorbertParser::StringexpContext *ctx) override {
        auto str = ctx->STRING()->getText();
        return expp(new StringExp(str));
    }

    virtual antlrcpp::Any visitTernaryexp(NorbertParser::TernaryexpContext *ctx) override {
        return expp(new TernaryExp(
            visit(ctx->exp(1)),
            visit(ctx->exp(0)),
            visit(ctx->exp(2))
        ));
    }

    virtual antlrcpp::Any visitParenexp(NorbertParser::ParenexpContext *ctx) override {
        return visit(ctx->exp());
    }

    virtual antlrcpp::Any visitInte(NorbertParser::InteContext *ctx) override {
        stringstream ss;
        if (ctx->INT()) ss << ctx->INT()->getText();
        else ss << ctx->HEX()->getText();
        int val;
        ss >> val;
        return expp(new IntExp(val));
    }

    virtual antlrcpp::Any visitExplist(NorbertParser::ExplistContext *ctx) override {
        vector<expp> l;
        for (auto e : ctx->exp()) {
            l.push_back(visit(e));
        }
        return l;
    }

    virtual antlrcpp::Any visitListexp(NorbertParser::ListexpContext *ctx) override {
        vector<expp> l = (ctx->explist())?visit(ctx->explist()).as<vector<expp>>():vector<expp>();
        return expp(new ListExp(l));
    }

    virtual antlrcpp::Any visitIndexexp(NorbertParser::IndexexpContext *ctx) override {
        return expp(new IndexExp(
            visit(ctx->exp(0)),
            visit(ctx->exp(1))
        ));
    }
};