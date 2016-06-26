#pragma once

#include <nixexpr.hh>

#include <memory>

class ExprVisitorBase
{
public:
    virtual void visit(nix::Expr * e) = 0;
};

class ExprVisitor : public ExprVisitorBase
{
public:
    virtual void visit(nix::Expr * e)
    {
        if (e == nullptr) { return visitNull(); }
        if (dynamic_cast<nix::ExprInt *>(e)) { return visit((nix::ExprInt *)e); }
        if (dynamic_cast<nix::ExprString *>(e)) { return visit((nix::ExprString *)e); }
        if (dynamic_cast<nix::ExprIndStr *>(e)) { return visit((nix::ExprIndStr *)e); }
        if (dynamic_cast<nix::ExprPath *>(e)) { return visit((nix::ExprPath *)e); }
        if (dynamic_cast<nix::ExprVar *>(e)) { return visit((nix::ExprVar *)e); }
        if (dynamic_cast<nix::ExprSelect *>(e)) { return visit((nix::ExprSelect *)e); }
        if (dynamic_cast<nix::ExprOpHasAttr *>(e)) { return visit((nix::ExprOpHasAttr *)e); }
        if (dynamic_cast<nix::ExprAttrs *>(e)) { return visit((nix::ExprAttrs *)e); }
        if (dynamic_cast<nix::ExprList *>(e)) { return visit((nix::ExprList *)e); }
        if (dynamic_cast<nix::ExprLambda *>(e)) { return visit((nix::ExprLambda *)e); }
        if (dynamic_cast<nix::ExprLet *>(e)) { return visit((nix::ExprLet *)e); }
        if (dynamic_cast<nix::ExprWith *>(e)) { return visit((nix::ExprWith *)e); }
        if (dynamic_cast<nix::ExprIf *>(e)) { return visit((nix::ExprIf *)e); }
        if (dynamic_cast<nix::ExprAssert *>(e)) { return visit((nix::ExprAssert *)e); }
        if (dynamic_cast<nix::ExprOpNot *>(e)) { return visit((nix::ExprOpNot *)e); }
        if (dynamic_cast<nix::ExprApp *>(e)) { return visit((nix::ExprApp *)e); }
        if (dynamic_cast<nix::ExprOpEq *>(e)) { return visit((nix::ExprOpEq *)e); }
        if (dynamic_cast<nix::ExprOpNEq *>(e)) { return visit((nix::ExprOpNEq *)e); }
        if (dynamic_cast<nix::ExprOpAnd *>(e)) { return visit((nix::ExprOpAnd *)e); }
        if (dynamic_cast<nix::ExprOpOr *>(e)) { return visit((nix::ExprOpOr *)e); }
        if (dynamic_cast<nix::ExprOpImpl *>(e)) { return visit((nix::ExprOpImpl *)e); }
        if (dynamic_cast<nix::ExprOpUpdate *>(e)) { return visit((nix::ExprOpUpdate *)e); }
        if (dynamic_cast<nix::ExprOpConcatLists *>(e)) { return visit((nix::ExprOpConcatLists *)e); }
        if (dynamic_cast<nix::ExprConcatStrings *>(e)) { return visit((nix::ExprConcatStrings *)e); }
        if (dynamic_cast<nix::ExprPos *>(e)) { return visit((nix::ExprPos *)e); }
        return visitNull();
    }

    virtual void visitNull() = 0;
    virtual void visitUnknown(nix::Expr *) = 0;
    virtual void visit(nix::ExprInt *) = 0;
    virtual void visit(nix::ExprString *) = 0;
    virtual void visit(nix::ExprIndStr *) = 0;
    virtual void visit(nix::ExprPath *) = 0;
    virtual void visit(nix::ExprVar *) = 0;
    virtual void visit(nix::ExprSelect *) = 0;
    virtual void visit(nix::ExprOpHasAttr *) = 0;
    virtual void visit(nix::ExprAttrs *) = 0;
    virtual void visit(nix::ExprList *) = 0;
    virtual void visit(nix::ExprLambda *) = 0;
    virtual void visit(nix::ExprLet *) = 0;
    virtual void visit(nix::ExprWith *) = 0;
    virtual void visit(nix::ExprIf *) = 0;
    virtual void visit(nix::ExprAssert *) = 0;
    virtual void visit(nix::ExprOpNot *) = 0;
    virtual void visit(nix::ExprApp *) = 0;
    virtual void visit(nix::ExprOpEq *) = 0;
    virtual void visit(nix::ExprOpNEq *) = 0;
    virtual void visit(nix::ExprOpAnd *) = 0;
    virtual void visit(nix::ExprOpImpl *) = 0;
    virtual void visit(nix::ExprOpUpdate *) = 0;
    virtual void visit(nix::ExprOpConcatLists *) = 0;
    virtual void visit(nix::ExprConcatStrings *) = 0;
    virtual void visit(nix::ExprPos *) = 0;
};

class ExprDefaultVisitor : public ExprVisitor
{
public:
    using ExprVisitor::visit;
    virtual void visitNull() { }
    virtual void visitUnknown(nix::Expr *) { }
    virtual void visit(nix::ExprInt *) { }
    virtual void visit(nix::ExprString *) { }
    virtual void visit(nix::ExprIndStr *) { }
    virtual void visit(nix::ExprPath *) { }
    virtual void visit(nix::ExprVar *) { }
    virtual void visit(nix::ExprSelect *) { }
    virtual void visit(nix::ExprOpHasAttr *) { }
    virtual void visit(nix::ExprAttrs *) { };
    virtual void visit(nix::ExprList *) { }
    virtual void visit(nix::ExprLambda *) { }
    virtual void visit(nix::ExprLet *) { }
    virtual void visit(nix::ExprWith *) { }
    virtual void visit(nix::ExprIf *) { }
    virtual void visit(nix::ExprAssert *) { }
    virtual void visit(nix::ExprOpNot *) { }
    virtual void visit(nix::ExprApp *) { }
    virtual void visit(nix::ExprOpEq *) { }
    virtual void visit(nix::ExprOpNEq *) { }
    virtual void visit(nix::ExprOpAnd *) { }
    virtual void visit(nix::ExprOpImpl *) { }
    virtual void visit(nix::ExprOpUpdate *) { }
    virtual void visit(nix::ExprOpConcatLists *) { }
    virtual void visit(nix::ExprConcatStrings *) { }
    virtual void visit(nix::ExprPos *) { }
};

class ExprClassIdentifier : public ExprVisitor
{
public:
    const char * result = "";

    using ExprVisitor::visit;
    virtual void visitNull() { result = "null"; }
    virtual void visitUnknown(nix::Expr *) { result = "unknown"; }
    virtual void visit(nix::ExprInt *) { result = "ExprInt"; }
    virtual void visit(nix::ExprString *) { result = "ExprString"; }
    virtual void visit(nix::ExprIndStr *) { result = "ExprIndStr"; }
    virtual void visit(nix::ExprPath *) { result = "ExprPath"; }
    virtual void visit(nix::ExprVar *) { result = "ExprVar"; }
    virtual void visit(nix::ExprSelect *) { result = "ExprSelect"; }
    virtual void visit(nix::ExprOpHasAttr *) { result = "ExprOpHasAttr"; }
    virtual void visit(nix::ExprAttrs *) { result = "ExprAttrs"; }
    virtual void visit(nix::ExprList *) { result = "ExprList"; }
    virtual void visit(nix::ExprLambda *) { result = "ExprLambda"; }
    virtual void visit(nix::ExprLet *) { result = "ExprLet"; }
    virtual void visit(nix::ExprWith *) { result = "ExprWith"; }
    virtual void visit(nix::ExprIf *) { result = "ExprIf"; }
    virtual void visit(nix::ExprAssert *) { result = "ExprAssert"; }
    virtual void visit(nix::ExprOpNot *) { result = "ExprOpNot"; }
    virtual void visit(nix::ExprApp *) { result = "ExprApp"; }
    virtual void visit(nix::ExprOpEq *) { result = "ExprOpEq"; }
    virtual void visit(nix::ExprOpNEq *) { result = "ExprOpNEq"; }
    virtual void visit(nix::ExprOpAnd *) { result = "ExprOpAnd"; }
    virtual void visit(nix::ExprOpImpl *) { result = "ExprOpImpl"; }
    virtual void visit(nix::ExprOpUpdate *) { result = "ExprOpUpdate"; }
    virtual void visit(nix::ExprOpConcatLists *) { result = "ExprOpConcatLists"; }
    virtual void visit(nix::ExprConcatStrings *) { result = "ExprConcatStrings"; }
    virtual void visit(nix::ExprPos *) { result = "ExprPos"; }
};

inline const char * exprClassName(nix::Expr * expr)
{
    ExprClassIdentifier id;
    id.visit(expr);
    return id.result;
}

class ExprDepthFirstSearch : public ExprVisitor
{
    ExprVisitorBase * v;

public:
    ExprDepthFirstSearch(ExprVisitorBase * innerVisitor)
    {
        this->v = innerVisitor;
    }

    using ExprVisitor::visit;
    virtual void visitNull() { v->visit(nullptr); }
    virtual void visitUnknown(nix::Expr * e) { v->visit(e); }
    virtual void visit(nix::ExprInt * e) { v->visit(e); }
    virtual void visit(nix::ExprString * e) { v->visit(e); }
    virtual void visit(nix::ExprIndStr * e) { v->visit(e); }
    virtual void visit(nix::ExprPath * e) { v->visit(e); }
    virtual void visit(nix::ExprVar * e) { v->visit(e); }
    virtual void visit(nix::ExprPos * e) { v->visit(e); }

    virtual void visit(nix::ExprSelect * e)
    {
        v->visit(e);
        visit(e->e);
        visit(e->def);
    }

    virtual void visit(nix::ExprOpHasAttr * e)
    {
        v->visit(e);
        visit(e->e);
    }

    virtual void visit(nix::ExprAttrs * e)
    {
        v->visit(e);
        for (auto & symbolAndAttr : e->attrs)
        {
            visit(symbolAndAttr.second.e);
        }
        for (auto & dynamicAttr : e->dynamicAttrs)
        {
            visit(dynamicAttr.nameExpr);
            visit(dynamicAttr.valueExpr);
        }
    }

    virtual void visit(nix::ExprList * e)
    {
        v->visit(e);
        for (nix::Expr * element : e->elems)
        {
            visit(element);
        }
    }

    virtual void visit(nix::ExprLambda * e)
    {
        v->visit(e);
        for (auto & formal : e->formals->formals)
        {
            visit(formal.def);
        }
        visit(e->body);
    }

    virtual void visit(nix::ExprLet * e)
    {
        v->visit(e);
        visit(e->body);
    }

    virtual void visit(nix::ExprWith * e)
    {
        v->visit(e);
        visit(e->attrs);
        visit(e->body);
    }

    virtual void visit(nix::ExprIf * e)
    {
        v->visit(e);
        visit(e->cond);
        visit(e->then);
        visit(e->else_);
    }

    virtual void visit(nix::ExprAssert * e)
    {
        v->visit(e);
        visit(e->cond);
        visit(e->body);
    }

    virtual void visit(nix::ExprOpNot * e) {
        v->visit(e);
        visit(e->e);
    }

    virtual void visit(nix::ExprApp * e)
    {
        v->visit(e);
        visit(e->e1);
        visit(e->e2);
    }

    virtual void visit(nix::ExprOpEq * e)
    {
        v->visit(e);
        visit(e->e1);
        visit(e->e2);
    }

    virtual void visit(nix::ExprOpNEq * e)
    {
        v->visit(e);
        visit(e->e1);
        visit(e->e2);
    }

    virtual void visit(nix::ExprOpAnd * e)
    {
        v->visit(e);
        visit(e->e1);
        visit(e->e2);
    }

    virtual void visit(nix::ExprOpImpl * e)
    {
        v->visit(e);
        visit(e->e1);
        visit(e->e2);
    }

    virtual void visit(nix::ExprOpUpdate * e)
    {
        v->visit(e);
        visit(e->e1);
        visit(e->e2);
    }

    virtual void visit(nix::ExprOpConcatLists * e)
    {
        v->visit(e);
        visit(e->e1);
        visit(e->e2);
    }

    virtual void visit(nix::ExprConcatStrings * e)
    {
        v->visit(e);
        if (e->es != nullptr)
        {
            for (auto & element : *(e->es))
            {
                visit(element);
            }
        }
    }
};

class ExprVisitorFunction : public ExprVisitorBase
{
public:
    typedef std::function<void(nix::Expr * e)> Function;

    Function fun;

    ExprVisitorFunction(Function fun)
    {
        this->fun = fun;
    }

    virtual void visit(nix::Expr * e)
    {
        if (e != nullptr)
        {
            fun(e);
        }
    }
};
